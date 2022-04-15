#pragma once

#include <cstddef>
#include <stdexcept>
#include "intrusive_tree.h"

struct left_tag;
struct right_tag;

template <typename Side>
using Other = std::conditional_t<
    std::is_same_v<Side, left_tag>,
    right_tag,
    left_tag>;

template <typename Left, typename Right,
          typename CompareLeft = std::less<Left>,
          typename CompareRight = std::less<Right>>
struct bimap {
  using left_t = Left;
  using right_t = Right;
  template <typename Side>
  using key_t = std::conditional_t<
      std::is_same_v<Side, left_tag>,
      left_t,
      right_t>;

  template <typename Side>
  using Comparator = std::conditional_t<
      std::is_same_v<Side, left_tag>,
      CompareLeft,
      CompareRight>;

private:
  struct node_t;
public:

  template <typename Side>
  struct base_iterator {
    friend bimap;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = key_t<Side>;
    using pointer = value_type const*;
    using reference = value_type const&;

    using tree_t = intr_tree<node_t, Side, key_t<Side>,
                                   Comparator<Side>>;
    using tree_other_t = intr_tree<node_t, Other<Side>, key_t<Other<Side>>,
                        Comparator<Other<Side>>>;

    base_iterator() = default;

    base_iterator(base_iterator const& other) : it(other.it), map_p(other.map_p)
    {}

    reference operator*() const {
      return it->template key<Side>();
    }
    pointer operator->() const {
      return &(it->template key<Side>());
    }

    base_iterator& operator++() {
      ++it;
      return *this;
    }
    base_iterator operator++(int) {
      base_iterator tmp(*this);
      ++*this;
      return tmp;
    }

    base_iterator& operator--() {
      --it;
      return *this;
    }
    base_iterator operator--(int) {
      base_iterator tmp(*this);
      --*this;
      return tmp;
    }

    base_iterator<Other<Side>> flip() const {
      if (it == map_p->template tree<Side>().end()) {
        return base_iterator<Other<Side>>(
            map_p->template tree<Other<Side>>().end(),
            map_p);
      }

      return base_iterator<Other<Side>>(
          tree_other_t::as_iterator(*it), map_p);
    }

    friend bool operator==(base_iterator const& a, base_iterator const& b) {
      return a.it == b.it;
    }

    friend bool operator!=(base_iterator const& a, base_iterator const& b) {
      return !(a == b);
    }

  private:
    typename tree_t::iterator it;
    bimap const* map_p;

    base_iterator(typename tree_t::iterator&& it, bimap const* p)
        : it(std::move(it)), map_p(p) {}
  };

  using right_iterator = base_iterator<right_tag>;
  using left_iterator = base_iterator<left_tag>;
  template <typename Side>
  using iterator = std::conditional_t<
      std::is_same_v<Side, left_tag>,
      left_iterator,
      right_iterator>;

  explicit bimap(CompareLeft compare_left = CompareLeft(),
                 CompareRight compare_right = CompareRight())
      : left_tree(compare_left),
        right_tree(compare_right) {}

  bimap(bimap const& other)
      : left_tree(other.left_tree),
        right_tree(other.right_tree) {
    copy_from(other);
  }
  bimap(bimap&& other) noexcept
      : left_tree(std::move(other.left_tree)),
        right_tree(std::move(other.right_tree)),
        size_(other.size_) {}

  bimap& operator=(bimap const &other) {
    if (this != &other) {
      clear();
      // change comparators:
      left_tree = other.left_tree;
      right_tree = other.right_tree;
      copy_from(other);
    }
    return *this;
  }
  bimap& operator=(bimap &&other) noexcept {
    if (this != &other) {
      clear();
      left_tree = std::move(other.left_tree);
      right_tree = std::move(other.right_tree);
      size_ = other.size_;
    }
    return *this;
  }

  void swap(bimap& other) {
    left_tree.swap(other.left_tree);
    right_tree.swap(other.right_tree);
    std::swap(size_, other.size_);
  }

  ~bimap() {
    clear();
  }

  left_iterator insert(left_t const& left, right_t const& right) {
    return insert_(left, right);
  }
  left_iterator insert(left_t const& left, right_t&& right) {
    return insert_(left, std::move(right));
  }
  left_iterator insert(left_t&& left, right_t const& right) {
    return insert_(std::move(left), right);
  }
  left_iterator insert(left_t&& left, right_t&& right) {
    return insert_(std::move(left), std::move(right));
  }

  left_iterator erase_left(left_iterator it) {
    return erase<left_tag>(it);
  }
  bool erase_left(left_t const& left) {
    return erase<left_tag>(left);
  }

  right_iterator erase_right(right_iterator it) {
    return erase<right_tag>(it);
  }
  bool erase_right(right_t const& right) {
    return erase<right_tag>(right);
  }

  left_iterator erase_left(left_iterator first, left_iterator last) {
    return erase<left_tag>(first, last);
  }
  right_iterator erase_right(right_iterator first, right_iterator last) {
      return erase<right_tag>(first, last);
  }

  left_iterator find_left(left_t const& left) const {
    return find<left_tag>(left);
  }
  right_iterator find_right(right_t const& right) const {
    return find<right_tag>(right);
  }

  right_t const& at_left(left_t const& key) const {
    return at<left_tag>(key);
  }
  left_t const& at_right(right_t const& key) const {
    return at<right_tag>(key);
  }

  template <typename U = right_t,
      typename = std::enable_if_t<std::is_default_constructible_v<U>>>
  right_t const& at_left_or_default(left_t const& key) {
    return at_or_default<left_tag>(key);
  }
  template <typename U = left_t,
      typename = std::enable_if_t<std::is_default_constructible_v<U>>>
  left_t const& at_right_or_default(right_t const& key) {
    return at_or_default<right_tag>(key);
  }

  left_iterator lower_bound_left(left_t const& left) const {
    return lower_bound<left_tag>(left);
  }
  left_iterator upper_bound_left(left_t const& left) const {
    return upper_bound<left_tag>(left);
  }

  right_iterator lower_bound_right(right_t const& right) const {
    return lower_bound<right_tag>(right);
  }
  right_iterator upper_bound_right(right_t const& right) const {
    return upper_bound<right_tag>(right);
  }

  left_iterator begin_left() const {
    return begin<left_tag>();
  }
  left_iterator end_left() const {
    return end<left_tag>();
  }

  right_iterator begin_right() const {
    return begin<right_tag>();
  }
  right_iterator end_right() const {
    return end<right_tag>();
  }

  bool empty() const {
    return left_tree.empty();
  }

  std::size_t size() const {
    return size_;
  }

  friend bool operator==(bimap const &a, bimap const &b) {
    if (a.size() != b.size()) {
      return false;
    }

    for (auto it_a = a.left_tree.begin(), it_b = b.left_tree.begin();
         it_a != a.left_tree.end(); ++it_a, ++it_b) {
      if (*it_a != *it_b) {
        return false;
      }
    }
    return true;
  }
  friend bool operator!=(bimap const &a, bimap const &b) {
    return !(a == b);
  }

private:
  struct node_t
      : tree_element<left_tag>,
        tree_element<right_tag> {
    node_t() = delete;

    template <typename L, typename R>
    node_t(L&& left, R&& right)
        : lr(std::forward<L>(left), std::forward<R>(right)) {}

    template <typename Side>
    key_t<Side>& key() {
      if constexpr (std::is_same_v<Side, left_tag>) {
        return lr.first;
      } else {
        return lr.second;
      }
    }

    template <typename Side>
    key_t<Side> const& key() const {
      if constexpr (std::is_same_v<Side, left_tag>) {
        return lr.first;
      } else {
        return lr.second;
      }
    }

    friend bool operator==(node_t const& a, node_t const& b) {
      return a.lr == b.lr;
    }

    friend bool operator!=(node_t const& a, node_t const& b) {
      return !(a == b);
    }

    private:
    std::pair<left_t, right_t> lr;
  };

  intr_tree<node_t, left_tag, key_t<left_tag>,
                  Comparator<left_tag>> left_tree;
  intr_tree<node_t, right_tag, key_t<right_tag>,
                  Comparator<right_tag>> right_tree;
  size_t size_{0};

  template <typename Side>
  intr_tree<node_t, Side, key_t<Side>, Comparator<Side>> const& tree() const {
    if constexpr (std::is_same_v<Side, left_tag>) {
      return left_tree;
    } else {
      return right_tree;
    }
  }

  template <typename Side>
  iterator<Side> begin() const {
    return iterator<Side>(tree<Side>().begin(), this);
  }

  template <typename Side>
  iterator<Side> end() const {
    return iterator<Side>(tree<Side>().end(), this);
  }

  template <typename Side>
  iterator<Side> erase(iterator<Side> it) {
    if (it == end<Side>()) {
      return it;
    }

    --size_;
    auto* node = (it.it).operator->();
    auto next = ++it;
    delete node;

    return next;
  }

  template <typename Side>
  bool erase(key_t<Side> const& key) {
    auto erase_res = erase<Side>(find<Side>(key));
    return !empty() ? erase_res != end<Side>() : erase_res == end<Side>();
  }

  template <typename Side>
  iterator<Side> erase(iterator<Side> first, iterator<Side> last) {
    auto it = first;
    while (it != last) {
      it = erase<Side>(it);
    }
    return it;
  }

  template <typename Side>
  iterator<Side> find(key_t<Side> const& key) const {
    return iterator<Side>(tree<Side>().find(key), this);
  }

  template <typename Side>
  key_t<Other<Side>> const& at(key_t<Side> const& key) const {
    auto it = find<Side>(key);
    if (it == end<Side>()) {
      throw std::out_of_range("No such element");
    }
    return *it.flip();
  }

  template <typename Side>
  iterator<Side> lower_bound(key_t<Side> const& key) const {
    return iterator<Side>(tree<Side>().lower_bound(key), this);
  }
  template <typename Side>
  iterator<Side> upper_bound(key_t<Side> const& key) const {
    return iterator<Side>(tree<Side>().upper_bound(key), this);
  }

  template <typename L, typename R>
  left_iterator insert_(L&& left, R&& right) {
    if (find_left(left) != end_left() || find_right(right) != end_right()) {
      return end_left();
    }

    return insert_no_check(std::forward<L>(left),
                           std::forward<R>(right));
  }

  template <typename L, typename R>
  left_iterator insert_no_check(L&& left, R&& right) {
    ++size_;
    auto* node = new node_t(std::forward<L>(left),
                            std::forward<R>(right));
    left_tree.insert(*node);
    right_tree.insert(*node);

    return left_iterator(left_tree.as_iterator(*node), this);
  }

  template <typename Side,
      typename = std::enable_if_t<
          std::is_default_constructible_v<key_t<Other<Side>>>>>
  key_t<Other<Side>> const& at_or_default(key_t<Side> const& key) {
    auto it_side = find<Side>(key);
    if (it_side != end<Side>()) {
      return *it_side.flip();
    } else {
      key_t<Other<Side>> other = key_t<Other<Side>>();
      auto it_other = find<Other<Side>>(other);

      if (it_other != end<Other<Side>>()) {
        it_other.it->template key<Side>() = key;
        return *it_other;
      } else {
        if constexpr (std::is_same_v<Side, left_tag>) {
          return *insert_no_check(key, std::move(other)).flip();
        } else {
          return *insert_no_check(std::move(other), key);
        }
      }
    }
  }

  void clear() {
    erase_left(begin_left(), end_left());
  }

  void copy_from(bimap const& other) {
    clear();
    for (auto it = other.left_tree.begin(); it != other.left_tree.end(); ++it) {
      insert(it->template key<left_tag>(), it->template key<right_tag>());
    }
  }
};
