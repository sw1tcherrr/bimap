#include "intrusive_tree.h"

// only for sentinel
base_tree_element::base_tree_element(base_tree_element&& other) noexcept
    : base_tree_element() {
  move_from(other);
}

// only for sentinel
base_tree_element& base_tree_element::operator=(base_tree_element&& other) noexcept {
  move_from(other);
  return *this;
}

// only for sentinel
void base_tree_element::move_from(base_tree_element& other) noexcept {
  link_left(this, other.left);
  link_right(this, other.right);
  other.left = other.right = other.parent = nullptr;
}

bool base_tree_element::in_tree() const noexcept {
  return !(parent == nullptr && left == nullptr && right == nullptr);
}

base_tree_element::~base_tree_element() {
  unlink();
}

void base_tree_element::link_left(base_tree_element* parent, base_tree_element* left) {
  if (parent) {
    parent->left = left;
    if (left) {
      left->parent = parent;
    }
  }
}

void base_tree_element::link_right(base_tree_element* parent, base_tree_element* right) {
  if (parent) {
    parent->right = right;
    if (right) {
      right->parent = parent;
    }
  }
}

base_tree_element* base_tree_element::max_in_subtree(base_tree_element* p) {
  while (p->right) {
    p = p->right;
  }
  return p;
}

base_tree_element* base_tree_element::min_in_subtree(base_tree_element* p) {
  while (p->left) {
    p = p->left;
  }
  return p;
}

base_tree_element* base_tree_element::prev(base_tree_element* p) {
  if (p->left) {
    return max_in_subtree(p->left);
  } else {
    while (p->is_left_child()) {
      p = p->parent;
    }
    p = p->parent;
  }

  return p;
}

base_tree_element* base_tree_element::next(base_tree_element* p) {
  if (p->right) {
    return min_in_subtree(p->right);
  } else {
    while (!p->is_left_child()) {
      p = p->parent;
    }
    p = p->parent;
  }

  return p;
}

bool base_tree_element::is_leaf() const noexcept {
  return left == nullptr && right == nullptr;
}

bool base_tree_element::has_one_child() const noexcept {
  return (left != nullptr && right == nullptr) ||
         (left == nullptr && right != nullptr);
}

bool base_tree_element::is_left_child() const noexcept {
  return parent->left == this;
}

base_tree_element* base_tree_element::get_only_child() noexcept {
  return (left != nullptr) ? left : right;
}

void base_tree_element::link_with_parent(base_tree_element* node) noexcept {
  if (is_left_child()) {
    link_left(parent, node);
  } else {
    link_right(parent, node);
  }
}

void base_tree_element::unlink() noexcept {
  if (!in_tree()) {
    return;
  }

  if (is_leaf() || has_one_child()) {
    link_with_parent(get_only_child());
  } else {
    auto* n = next(this);
    n->unlink();
    link_left(n, left);
    link_right(n, right);
    link_with_parent(n);

    left = right = parent = nullptr;
  }
}
