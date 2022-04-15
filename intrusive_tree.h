#pragma once

#include <cstddef>
#include <iterator>

struct base_tree_element {
    template <typename T, typename Key, typename Tag, typename Comparator>
    friend struct intr_tree;

    base_tree_element() noexcept = default;

    base_tree_element(base_tree_element const&) = delete;
    base_tree_element& operator=(base_tree_element const&) = delete;

    base_tree_element(base_tree_element&& other) noexcept;

    base_tree_element& operator=(base_tree_element&& other) noexcept;

    bool in_tree() const noexcept;

    virtual ~base_tree_element();

   private:
    base_tree_element* left{nullptr};
    base_tree_element* right{nullptr};
    base_tree_element* parent{nullptr};

    void move_from(base_tree_element& other) noexcept;

    static void link_left(base_tree_element* parent, base_tree_element* left);

    static void link_right(base_tree_element* parent, base_tree_element* right);

    void link_with_parent(base_tree_element* node) noexcept;

    void unlink() noexcept;

    static base_tree_element* next(base_tree_element* p);

    static base_tree_element* prev(base_tree_element* p);

    static base_tree_element* min_in_subtree(base_tree_element* p);

    static base_tree_element* max_in_subtree(base_tree_element* p);

    bool is_leaf() const noexcept;

    bool has_one_child() const noexcept;

    bool is_left_child() const noexcept;

    base_tree_element* get_only_child() noexcept;
};

template <typename Tag>
struct tree_element : base_tree_element {};

template <typename Elt, typename Tag, typename Key, typename Comparator>
struct intr_tree : Comparator {
  using untagged = base_tree_element;
  using tagged = tree_element<Tag>;

  struct iterator {
    friend intr_tree;

    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Elt;
    using pointer = Elt*;
    using reference = Elt&;

    iterator() = default;
    iterator(iterator const& other) : ptr(other.ptr) {}

    reference operator*() const {
      return *static_cast<pointer>(static_cast<tagged*>(ptr));
    }
    pointer operator->() const {
      return static_cast<pointer>(static_cast<tagged*>(ptr));
    }

    iterator& operator++() & {
      ptr = untagged::next(ptr);
      return *this;
    }
    iterator operator++(int) & {
      iterator tmp(*this);
      ++*this;
      return tmp;
    }

    iterator& operator--() & {
      ptr = untagged::prev(ptr);
      return *this;
    }
    iterator operator--(int) & {
      iterator tmp(*this);
      --*this;
      return tmp;
    }

    friend bool operator==(iterator const& a, iterator const& b) {
      return a.ptr == b.ptr;
    }

    friend bool operator!=(iterator const& a, iterator const& b) {
      return a.ptr != b.ptr;
    }

  private:
    untagged* ptr{nullptr};
    explicit iterator(untagged const* ptr) : ptr(const_cast<untagged*>(ptr)) {}
  };

  explicit intr_tree(Comparator comparator = Comparator()) noexcept
      : Comparator(comparator) {}

  // only for comparator
  intr_tree(intr_tree const& other) : Comparator(other) {}
  intr_tree& operator=(intr_tree const& other) {
    Comparator::operator=(other);
    return *this;
  }

  intr_tree(intr_tree&& other) noexcept : root(std::move(other.root)) {}

  intr_tree& operator=(intr_tree&& other) noexcept {
    clear();
    root = std::move(other.root);
    return *this;
  }

  void swap(intr_tree& other) {
    std::swap(root, other.root);
  }

  ~intr_tree() {
    clear();
  }

  bool empty() const {
    return root.left == nullptr && root.right == nullptr;
  }

  static iterator as_iterator(Elt const& elt) noexcept {
    return iterator(&static_cast<tagged&>(const_cast<Elt&>(elt)));
  }

  iterator find(Key const& key) const noexcept {
    untagged* cur = root.left;
    for (;;) {
      if (cur == nullptr) {
        return end();
      }

      if (cmp_key(get_key(cur), key)) {
        cur = cur->right;
      } else if (cmp_key(key, get_key(cur))) {
        cur = cur->left;
      } else {
        return iterator(cur);
      }
    }
  }

  iterator insert(Elt const& elt) {
    untagged* cur = root.left;
    untagged* elt_p = static_cast<tagged*>(const_cast<Elt*>(&elt));
    if (cur == nullptr) {
      untagged::link_left(&root, elt_p);
      return iterator(cur);
    }

    for (;;) {
      if (cmp_node(cur, elt_p)) {
        if (cur->right) {
          cur = cur->right;
        } else {
          untagged::link_right(cur, elt_p);
          return as_iterator(elt);
        }
      } else if (cmp_node(elt_p, cur)) {
        if (cur->left) {
          cur = cur->left;
        } else {
          untagged::link_left(cur, elt_p);
          return as_iterator(elt);
        }
      } else {
        return end();
      }
    }
  }

  iterator erase(Elt const& elt) noexcept  {
    auto it = as_iterator(elt);
    elt.unlink();
    return ++it;
  }

  iterator lower_bound(Key const& key) const {
    untagged const* res = &root;
    untagged* cur = root.left;
    for (;;) {
      if (cur == nullptr) {
        break;
      }
      if (cmp_key(get_key(cur), key)) {
        cur = cur->right;
      } else {
        res = cur;
        cur = cur->left;
      }
    }

    return iterator(res);
  }

  iterator upper_bound(Key const& key) const {
    untagged const* res = &root;
    untagged* cur = root.left;
    for (;;) {
      if (cur == nullptr) {
        break;
      }
      if (cmp_key(key, get_key(cur))) {
        res = cur;
        cur = cur->left;
      } else {
        cur = cur->right;
      }
    }

    return iterator(res);
  }

  iterator begin() const {
    return iterator(untagged::min_in_subtree(&root));
  }

  iterator end() const {
    return iterator(&root);
  }

private:
  mutable untagged root;

  void delete_subtree(untagged* p) {
    if (p) {
      delete_subtree(p->left);
      delete_subtree(p->right);
      p->unlink();
    }
  }

  void clear() {
    delete_subtree(&root);
  }

  bool cmp_key(Key const& a, Key const& b) const {
    return Comparator::operator()(a, b);
  }

  bool cmp_node(untagged const* a, untagged const* b) const {
    return cmp_key(get_key(a), get_key(b));
  }

  Key const& get_key(untagged const* node) const {
    return static_cast<Elt const*>(static_cast<tagged const*>(node))->template key<Tag>();
  }
};
