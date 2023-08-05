#ifndef range_tree_h__
#define range_tree_h__

#include <stdint.h>
#include <type_traits>
#include <vector>
#include <optional>
#include <algorithm>
#include <deque>

template <typename TKey, size_t TDimensions, typename TValue>
struct dim_point
{
  TKey p[TDimensions];
  TValue value;

  static constexpr size_t dim = TDimensions;
};

template <typename TKey, typename TValue>
struct range_tree
{
  typedef dim_point<TKey, 2, TValue> point;

private:
  struct node
  {
    TKey key;
    TKey min, max;

    size_t left = (size_t)-1, right = (size_t)-1;

    node(const TKey k, const TKey min, const TKey max) : key(k), min(min), max(max) {}
  };

  struct hdim_ref
  {
    TKey coord;
    size_t left = (size_t)-1, right = (size_t)-1;
    size_t pointIndex;

    hdim_ref(const TKey coord, const size_t pointIndex) : coord(coord), pointIndex(pointIndex) { }
  };

  struct hdim_node
  {
    std::vector<hdim_ref> keys; // uses same left/right index as `dim_0`.

    hdim_node(const point *pP, const size_t start, const size_t end, const size_t dim)
    {
      for (size_t i = start; i < end; i++)
        keys.push_back(hdim_ref(pP[i].p[dim], i));
    }
  };

  std::vector<node> dim_0;
  std::vector<hdim_node> dim_1;
  std::vector<point> x_sorted;

  inline void find_in_range(const TKey min[point::dim], const TKey max[point::dim], const size_t nodeIdx, const size_t y_start, std::vector<point> &out) const
  {
    const node &n = dim_0[nodeIdx];

    if (n.min >= min[0] && n.max <= max[0])
    {
      // Check y dim.
      const hdim_node &y = dim_1[nodeIdx];

      for (size_t i = y_start; i < y.keys.size(); i++)
      {
        if (y.keys[i].coord > max[1])
          return;

        out.push_back(x_sorted[y.keys[i].pointIndex]);
      }
    }
    else
    {
      // Go left?
      if (n.left != (size_t)-1 && n.min <= max[0] && n.key >= min[0])
        find_in_range(min, max, n.left, dim_1[nodeIdx].keys[y_start].left, out);

      // Go right?
      if (n.right != (size_t)-1 && n.max >= min[0] && n.key <= max[0])
        find_in_range(min, max, n.right, dim_1[nodeIdx].keys[y_start].right, out);
    }
  }

public:
  static inline bool create(const point *pValues, const size_t valueCount, range_tree<TKey, TValue> &tree)
  {
    if (valueCount == 0)
      return false;

    bool failed = false;

    {
      std::vector<point> x_sorted(pValues, pValues + valueCount);

      std::sort(x_sorted.begin(), x_sorted.end(), [&](const point &a, const point &b)
        {
          for (size_t i = 0; i < point::dim; i++)
          {
            if (a.p[i] < b.p[i])
              return true;

            if (a.p[i] > b.p[i])
              return false;
          }

          failed = true;
          return false;
        });

      if (failed)
        return false;

      tree.x_sorted = std::move(x_sorted);
    }

    struct sub_range_x
    {
      size_t parentIdx;
      bool isLeft;
      size_t min, max; // max is exclusive.

      sub_range_x(const size_t parent, const bool isLeft, const size_t min, const size_t max) :
        parentIdx(parent),
        isLeft(isLeft),
        min(min),
        max(max)
      { }
    };

    std::deque<sub_range_x> queue;

    // Add root.
    {
      const size_t splitPoint = tree.x_sorted.size() / 2;
      tree.dim_0.push_back(node(tree.x_sorted[splitPoint].p[0], tree.x_sorted.front().p[0], tree.x_sorted.back().p[0]));
      tree.dim_1.push_back(hdim_node(&tree.x_sorted[0], 0, tree.x_sorted.size(), 1));

      if (tree.x_sorted.size() > 1)
      {
        const size_t idx = tree.dim_0.size() - 1;

        queue.push_back(sub_range_x(idx, true, 0, splitPoint));
        queue.push_back(sub_range_x(idx, false, splitPoint, tree.x_sorted.size()));
      }
    }

    while (queue.size())
    {
      const sub_range_x child = queue.front();
      queue.pop_front();

      const size_t splitPoint = (child.max - child.min) / 2 + child.min;

      tree.dim_0.push_back(node(tree.x_sorted[splitPoint].p[0], tree.x_sorted[child.min].p[0], tree.x_sorted[child.max - 1].p[0]));
      tree.dim_1.push_back(hdim_node(&tree.x_sorted[0], child.min, child.max, 1));

      const size_t idx = tree.dim_0.size() - 1;

      if (child.isLeft)
        tree.dim_0[child.parentIdx].left = idx;
      else
        tree.dim_0[child.parentIdx].right = idx;

      if (child.min + 1 != child.max)
      {
        queue.push_back(sub_range_x(idx, true, child.min, splitPoint));
        queue.push_back(sub_range_x(idx, false, splitPoint, child.max));
      }
    }

    for (size_t i = 0; i < tree.dim_1.size(); i++)
    {
      hdim_node &n = tree.dim_1[i];

      std::sort(n.keys.begin(), n.keys.end(), [&](const hdim_ref &a, const hdim_ref &b)
        {
          if (a.coord < b.coord)
            return true;

          if (a.coord > b.coord) // please, dear compiler, get rid of this check.
            return false;

          // if porting to more than 2 dimensions, you'll have to do some sorting based on the other dimensions.

          return false;
        });
    }

    for (size_t i = 0; i < tree.dim_1.size(); i++)
    {
      const node &n0 = tree.dim_0[i];

      if (n0.left == (size_t)-1) // If this has no children.
        continue;

      hdim_node &n = tree.dim_1[i];

      size_t inLeft = 0, inRight = 0;
      const std::vector<hdim_ref> &left = tree.dim_1[n0.left].keys;
      const std::vector<hdim_ref> &right = tree.dim_1[n0.right].keys;

      for (hdim_ref &k : n.keys)
      {
        // Left.
        {
          while (inLeft < left.size() && left[inLeft].coord < k.coord)
            inLeft++;

          k.left = (inLeft >= left.size() ? (size_t)-1 : inLeft);
        }

        // Right.
        {
          while (inRight < right.size() && right[inRight].coord < k.coord)
            inRight++;

          k.right = (inRight >= right.size() ? (size_t)-1 : inRight);
        }
      }
    }

    return true;
  }

  inline std::vector<point> in_range(const TKey min[point::dim], const TKey max[point::dim]) const
  {
    std::vector<point> ret;

    const auto &it = std::lower_bound(dim_1[0].keys.begin(), dim_1[0].keys.end(), min[1], [](const hdim_ref &a, const TKey &b)
      {
        if (a.coord < b)
          return true;

        if (a.coord > b) // please, dear compiler, get rid of this check.
          return false;

        // if porting to more than 2 dimensions, you'll have to do some sorting based on the other dimensions.
        
        return false;
      });
    
    if (it == dim_1[0].keys.end())
      return ret;

    const size_t y_start = it - dim_1[0].keys.begin();

    find_in_range(min, max, 0, y_start, ret);
    
    return ret;
  }
};

#endif // range_tree_h__
