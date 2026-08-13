#ifndef SJTU_PRIORITY_QUEUE_HPP
#define SJTU_PRIORITY_QUEUE_HPP

#include <cstddef>
#include <functional>
#include "exceptions.hpp"

namespace sjtu {
/**
 * @brief a container like std::priority_queue which is a heap internal.
 * **Exception Safety**: The `Compare` operation might throw exceptions for certain data.
 * In such cases, any ongoing operation should be terminated, and the priority queue should be restored to its original state before the operation began.
 *
 * Implementation: a leftist (skew) heap, providing O(log n) merge while keeping the
 * heap property. The merge routine never mutates its inputs until it fully succeeds,
 * which gives strong exception safety when `Compare` throws.
 */
template<typename T, class Compare = std::less<T>>
class priority_queue {
private:
	struct Node {
		T val;
		Node *left;
		Node *right;
		int npl; // null-path length (null node == -1, leaf == 0)

		Node(const T &v) : val(v), left(nullptr), right(nullptr), npl(0) {}
	};

	Node *root;
	size_t currentSize;

	// Recompute the leftist property for node x (ensure left.npl >= right.npl).
	static void updateNPL(Node *x) {
		int l = (x->left) ? x->left->npl : -1;
		int r = (x->right) ? x->right->npl : -1;
		if (l < r) {
			Node *t = x->left;
			x->left = x->right;
			x->right = t;
		}
		x->npl = ((x->right) ? x->right->npl : -1) + 1;
	}

	// Merge two leftist heaps. Does NOT mutate a or b until the whole merge
	// fully succeeds; the only point that can throw is the `Compare` call, which
	// happens strictly before any pointer mutation. Returns the new root.
	static Node *merge(Node *a, Node *b, Compare &comp) {
		if (!a) return b;
		if (!b) return a;
		// Make `a` the node with the higher priority (larger value for max-heap).
		if (comp(a->val, b->val)) {
			Node *t = a;
			a = b;
			b = t;
		}
		a->right = merge(a->right, b, comp);
		updateNPL(a);
		return a;
	}

	// Deep copy of a subtree. Exception safe: on failure all allocated nodes are freed.
	static Node *copyTree(Node *x, Compare &comp) {
		if (!x) return nullptr;
		Node *left = copyTree(x->left, comp);
		try {
			Node *right = copyTree(x->right, comp);
			try {
				Node *n = new Node(x->val);
				n->npl = x->npl;
				n->left = left;
				n->right = right;
				return n;
			} catch (...) {
				delete right;
				throw;
			}
		} catch (...) {
			delete left;
			throw;
		}
	}

	static void clearTree(Node *x) {
		if (!x) return;
		clearTree(x->left);
		clearTree(x->right);
		delete x;
	}

	void swapData(priority_queue &other) {
		Node *troot = root;
		root = other.root;
		other.root = troot;
		size_t tsz = currentSize;
		currentSize = other.currentSize;
		other.currentSize = tsz;
	}

public:
	/**
	 * @brief default constructor
	 */
	priority_queue() : root(nullptr), currentSize(0) {}

	/**
	 * @brief copy constructor
	 * @param other the priority_queue to be copied
	 */
	priority_queue(const priority_queue &other) : root(nullptr), currentSize(0) {
		Compare comp;
		root = copyTree(other.root, comp);
		currentSize = other.currentSize;
	}

	/**
	 * @brief deconstructor
	 */
	~priority_queue() {
		clearTree(root);
	}

	/**
	 * @brief Assignment operator
	 * @param other the priority_queue to be assigned from
	 * @return a reference to this priority_queue after assignment
	 */
	priority_queue &operator=(const priority_queue &other) {
		if (this != &other) {
			priority_queue tmp(other); // copy; may throw, but *this is untouched
			swapData(tmp);
		}
		return *this;
	}

	/**
	 * @brief get the top element of the priority queue.
	 * @return a reference of the top element.
	 * @throws container_is_empty if empty() returns true
	 */
	const T & top() const {
		if (empty()) throw container_is_empty();
		return root->val;
	}

	/**
	 * @brief push new element to the priority queue.
	 * @param e the element to be pushed
	 */
	void push(const T &e) {
		Node *n = new Node(e); // may throw (copy of T) before any mutation
		Compare comp;
		try {
			root = merge(root, n, comp);
		} catch (...) {
			delete n; // node was not attached yet
			throw;
		}
		++currentSize;
	}

	/**
	 * @brief delete the top element from the priority queue.
	 * @throws container_is_empty if empty() returns true
	 */
	void pop() {
		if (empty()) throw container_is_empty();
		Node *old = root;
		Compare comp;
		root = merge(old->left, old->right, comp); // exception safe: root unchanged on throw
		delete old;
		--currentSize;
	}

	/**
	 * @brief return the number of elements in the priority queue.
	 * @return the number of elements.
	 */
	size_t size() const {
		return currentSize;
	}

	/**
	 * @brief check if the container is empty.
	 * @return true if it is empty, false otherwise.
	 */
	bool empty() const {
		return currentSize == 0;
	}

	/**
	 * @brief merge another priority_queue into this one.
	 * The other priority_queue will be cleared after merging.
	 * The complexity is at most O(logn).
	 * @param other the priority_queue to be merged.
	 */
	void merge(priority_queue &other) {
		if (this == &other) return;
		Compare comp;
		root = merge(root, other.root, comp); // exception safe: on throw, both queues unchanged
		other.root = nullptr;
		currentSize += other.currentSize;
		other.currentSize = 0;
	}
};

}

#endif
