//==-- llvm/ADT/ilist.h - Intrusive Linked List Template ---------*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines classes to implement an intrusive doubly linked list class
// (i.e. each node of the list must contain a next and previous field for the
// list.
//
// The ilist class itself should be a plug in replacement for list.  This list
// replacement does not provide a constant time size() method, so be careful to
// use empty() when you really want to know if it's empty.
//
// The ilist class is implemented as a circular list.  The list itself contains
// a sentinel node, whose Next points at begin() and whose Prev points at
// rbegin().  The sentinel node itself serves as end() and rend().
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ADT_ILIST_H
#define LLVM_ADT_ILIST_H

#include "llvm/ADT/ilist_base.h"
#include "llvm/ADT/ilist_iterator.h"
#include "llvm/ADT/ilist_node.h"
#include "llvm/Support/Compiler.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace llvm {

template<typename NodeTy, typename Traits> class iplist;

namespace ilist_detail {

template <class T> T &make();

/// Type trait to check for a traits class that has a getNext member (as a
/// canary for any of the ilist_nextprev_traits API).
template <class TraitsT, class NodeT> struct HasGetNext {
  typedef char Yes[1];
  typedef char No[2];
  template <size_t N> struct SFINAE {};

  template <class U>
  static Yes &test(U *I, decltype(I->getNext(&make<NodeT>())) * = 0);
  template <class> static No &test(...);

public:
  static const bool value = sizeof(test<TraitsT>(nullptr)) == sizeof(Yes);
};

/// Type trait to check for a traits class that has a createSentinel member (as
/// a canary for any of the ilist_sentinel_traits API).
template <class TraitsT> struct HasCreateSentinel {
  typedef char Yes[1];
  typedef char No[2];

  template <class U>
  static Yes &test(U *I, decltype(I->createSentinel()) * = 0);
  template <class> static No &test(...);

public:
  static const bool value = sizeof(test<TraitsT>(nullptr)) == sizeof(Yes);
};

template <class TraitsT, class NodeT> struct HasObsoleteCustomization {
  static const bool value =
      HasGetNext<TraitsT, NodeT>::value || HasCreateSentinel<TraitsT>::value;
};

} // end namespace ilist_detail

template <typename NodeTy> struct ilist_traits;

/// ilist_node_traits - A fragment for template traits for intrusive list
/// that provides default node related operations.
///
template<typename NodeTy>
struct ilist_node_traits {
  static NodeTy *createNode(const NodeTy &V) { return new NodeTy(V); }
  static void deleteNode(NodeTy *V) { delete V; }

  void addNodeToList(NodeTy *) {}
  void removeNodeFromList(NodeTy *) {}
  void transferNodesFromList(ilist_node_traits &    /*SrcTraits*/,
                             ilist_iterator<NodeTy> /*first*/,
                             ilist_iterator<NodeTy> /*last*/) {}
};

/// ilist_default_traits - Default template traits for intrusive list.
/// By inheriting from this, you can easily use default implementations
/// for all common operations.
///
template <typename NodeTy>
struct ilist_default_traits : public ilist_node_traits<NodeTy> {};

// Template traits for intrusive list.  By specializing this template class, you
// can change what next/prev fields are used to store the links...
template<typename NodeTy>
struct ilist_traits : public ilist_default_traits<NodeTy> {};

// Const traits are the same as nonconst traits...
template<typename Ty>
struct ilist_traits<const Ty> : public ilist_traits<Ty> {};

//===----------------------------------------------------------------------===//
//
/// The subset of list functionality that can safely be used on nodes of
/// polymorphic types, i.e. a heterogeneous list with a common base class that
/// holds the next/prev pointers.  The only state of the list itself is an
/// ilist_sentinel, which holds pointers to the first and last nodes in the
/// list.
template <typename NodeTy, typename Traits = ilist_traits<NodeTy>>
class iplist : public Traits, ilist_base, ilist_node_access {
  // TODO: Drop this assertion and the transitive type traits anytime after
  // v4.0 is branched (i.e,. keep them for one release to help out-of-tree code
  // update).
  static_assert(!ilist_detail::HasObsoleteCustomization<Traits, NodeTy>::value,
                "ilist customization points have changed!");

  ilist_sentinel<NodeTy> Sentinel;

  typedef ilist_node<NodeTy> node_type;
  typedef const ilist_node<NodeTy> const_node_type;

  static bool op_less(NodeTy &L, NodeTy &R) { return L < R; }
  static bool op_equal(NodeTy &L, NodeTy &R) { return L == R; }

  // Copying intrusively linked nodes doesn't make sense.
  iplist(const iplist &) = delete;
  void operator=(const iplist &) = delete;

public:
  typedef NodeTy *pointer;
  typedef const NodeTy *const_pointer;
  typedef NodeTy &reference;
  typedef const NodeTy &const_reference;
  typedef NodeTy value_type;
  typedef ilist_iterator<NodeTy> iterator;
  typedef ilist_iterator<const NodeTy> const_iterator;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef ilist_iterator<const NodeTy, true> const_reverse_iterator;
  typedef ilist_iterator<NodeTy, true> reverse_iterator;

  iplist() = default;
  ~iplist() { clear(); }

  // Iterator creation methods.
  iterator begin() { return ++iterator(Sentinel); }
  const_iterator begin() const { return ++const_iterator(Sentinel); }
  iterator end() { return iterator(Sentinel); }
  const_iterator end() const { return const_iterator(Sentinel); }

  // reverse iterator creation methods.
  reverse_iterator rbegin()            { return ++reverse_iterator(Sentinel); }
  const_reverse_iterator rbegin() const{ return ++const_reverse_iterator(Sentinel); }
  reverse_iterator rend()              { return reverse_iterator(Sentinel); }
  const_reverse_iterator rend() const { return const_reverse_iterator(Sentinel); }

  // Miscellaneous inspection routines.
  size_type max_size() const { return size_type(-1); }
  bool LLVM_ATTRIBUTE_UNUSED_RESULT empty() const { return Sentinel.empty(); }

  // Front and back accessor functions...
  reference front() {
    assert(!empty() && "Called front() on empty list!");
    return *begin();
  }
  const_reference front() const {
    assert(!empty() && "Called front() on empty list!");
    return *begin();
  }
  reference back() {
    assert(!empty() && "Called back() on empty list!");
    return *--end();
  }
  const_reference back() const {
    assert(!empty() && "Called back() on empty list!");
    return *--end();
  }

  void swap(iplist &RHS) {
    assert(0 && "Swap does not use list traits callback correctly yet!");
    std::swap(Sentinel, RHS.Sentinel);
  }

  iterator insert(iterator where, NodeTy *New) {
    ilist_base::insertBefore(*where.getNodePtr(), *this->getNodePtr(New));

    this->addNodeToList(New);  // Notify traits that we added a node...
    return iterator(New);
  }

  iterator insert(iterator where, const NodeTy &New) {
    return this->insert(where, new NodeTy(New));
  }

  iterator insertAfter(iterator where, NodeTy *New) {
    if (empty())
      return insert(begin(), New);
    else
      return insert(++where, New);
  }

  NodeTy *remove(iterator &IT) {
    assert(IT != end() && "Cannot remove end of list!");
    NodeTy *Node = &*IT++;
    ilist_base::remove(*this->getNodePtr(Node));
    this->removeNodeFromList(Node);  // Notify traits that we removed a node...
    return Node;
  }

  NodeTy *remove(const iterator &IT) {
    iterator MutIt = IT;
    return remove(MutIt);
  }

  NodeTy *remove(NodeTy *IT) { return remove(iterator(IT)); }
  NodeTy *remove(NodeTy &IT) { return remove(iterator(IT)); }

  // erase - remove a node from the controlled sequence... and delete it.
  iterator erase(iterator where) {
    this->deleteNode(remove(where));
    return where;
  }

  iterator erase(NodeTy *IT) { return erase(iterator(IT)); }
  iterator erase(NodeTy &IT) { return erase(iterator(IT)); }

  /// Remove all nodes from the list like clear(), but do not call
  /// removeNodeFromList() or deleteNode().
  ///
  /// This should only be used immediately before freeing nodes in bulk to
  /// avoid traversing the list and bringing all the nodes into cache.
  void clearAndLeakNodesUnsafely() { Sentinel.reset(); }

private:
  // transfer - The heart of the splice function.  Move linked list nodes from
  // [first, last) into position.
  //
  void transfer(iterator position, iplist &L2, iterator first, iterator last) {
    if (position == last)
      return;

    ilist_base::transferBefore(*position.getNodePtr(), *first.getNodePtr(),
                               *last.getNodePtr());

    // Callback.  Note that the nodes have moved from before-last to
    // before-position.
    this->transferNodesFromList(L2, first, position);
  }

public:

  //===----------------------------------------------------------------------===
  // Functionality derived from other functions defined above...
  //

  size_type LLVM_ATTRIBUTE_UNUSED_RESULT size() const {
    return std::distance(begin(), end());
  }

  iterator erase(iterator first, iterator last) {
    while (first != last)
      first = erase(first);
    return last;
  }

  void clear() { erase(begin(), end()); }

  // Front and back inserters...
  void push_front(NodeTy *val) { insert(begin(), val); }
  void push_back(NodeTy *val) { insert(end(), val); }
  void pop_front() {
    assert(!empty() && "pop_front() on empty list!");
    erase(begin());
  }
  void pop_back() {
    assert(!empty() && "pop_back() on empty list!");
    iterator t = end(); erase(--t);
  }

  // Special forms of insert...
  template<class InIt> void insert(iterator where, InIt first, InIt last) {
    for (; first != last; ++first) insert(where, *first);
  }

  // Splice members - defined in terms of transfer...
  void splice(iterator where, iplist &L2) {
    if (!L2.empty())
      transfer(where, L2, L2.begin(), L2.end());
  }
  void splice(iterator where, iplist &L2, iterator first) {
    iterator last = first; ++last;
    if (where == first || where == last) return; // No change
    transfer(where, L2, first, last);
  }
  void splice(iterator where, iplist &L2, iterator first, iterator last) {
    if (first != last) transfer(where, L2, first, last);
  }
  void splice(iterator where, iplist &L2, NodeTy &N) {
    splice(where, L2, iterator(N));
  }
  void splice(iterator where, iplist &L2, NodeTy *N) {
    splice(where, L2, iterator(N));
  }

  template <class Compare>
  void merge(iplist &Right, Compare comp) {
    if (this == &Right)
      return;
    iterator First1 = begin(), Last1 = end();
    iterator First2 = Right.begin(), Last2 = Right.end();
    while (First1 != Last1 && First2 != Last2) {
      if (comp(*First2, *First1)) {
        iterator Next = First2;
        transfer(First1, Right, First2, ++Next);
        First2 = Next;
      } else {
        ++First1;
      }
    }
    if (First2 != Last2)
      transfer(Last1, Right, First2, Last2);
  }
  void merge(iplist &Right) { return merge(Right, op_less); }

  template <class Compare>
  void sort(Compare comp) {
    // The list is empty, vacuously sorted.
    if (empty())
      return;
    // The list has a single element, vacuously sorted.
    if (std::next(begin()) == end())
      return;
    // Find the split point for the list.
    iterator Center = begin(), End = begin();
    while (End != end() && std::next(End) != end()) {
      Center = std::next(Center);
      End = std::next(std::next(End));
    }
    // Split the list into two.
    iplist RightHalf;
    RightHalf.splice(RightHalf.begin(), *this, Center, end());

    // Sort the two sublists.
    sort(comp);
    RightHalf.sort(comp);

    // Merge the two sublists back together.
    merge(RightHalf, comp);
  }
  void sort() { sort(op_less); }

  /// \brief Get the previous node, or \c nullptr for the list head.
  NodeTy *getPrevNode(NodeTy &N) const {
    auto I = N.getIterator();
    if (I == begin())
      return nullptr;
    return &*std::prev(I);
  }
  /// \brief Get the previous node, or \c nullptr for the list head.
  const NodeTy *getPrevNode(const NodeTy &N) const {
    return getPrevNode(const_cast<NodeTy &>(N));
  }

  /// \brief Get the next node, or \c nullptr for the list tail.
  NodeTy *getNextNode(NodeTy &N) const {
    auto Next = std::next(N.getIterator());
    if (Next == end())
      return nullptr;
    return &*Next;
  }
  /// \brief Get the next node, or \c nullptr for the list tail.
  const NodeTy *getNextNode(const NodeTy &N) const {
    return getNextNode(const_cast<NodeTy &>(N));
  }
};


template<typename NodeTy>
struct ilist : public iplist<NodeTy> {
  typedef typename iplist<NodeTy>::size_type size_type;
  typedef typename iplist<NodeTy>::iterator iterator;

  ilist() {}
  ilist(const ilist &right) : iplist<NodeTy>() {
    insert(this->begin(), right.begin(), right.end());
  }
  explicit ilist(size_type count) {
    insert(this->begin(), count, NodeTy());
  }
  ilist(size_type count, const NodeTy &val) {
    insert(this->begin(), count, val);
  }
  template<class InIt> ilist(InIt first, InIt last) {
    insert(this->begin(), first, last);
  }

  // bring hidden functions into scope
  using iplist<NodeTy>::insert;
  using iplist<NodeTy>::push_front;
  using iplist<NodeTy>::push_back;

  // Main implementation here - Insert for a node passed by value...
  iterator insert(iterator where, const NodeTy &val) {
    return insert(where, this->createNode(val));
  }


  // Front and back inserters...
  void push_front(const NodeTy &val) { insert(this->begin(), val); }
  void push_back(const NodeTy &val) { insert(this->end(), val); }

  void insert(iterator where, size_type count, const NodeTy &val) {
    for (; count != 0; --count) insert(where, val);
  }

  // Assign special forms...
  void assign(size_type count, const NodeTy &val) {
    iterator I = this->begin();
    for (; I != this->end() && count != 0; ++I, --count)
      *I = val;
    if (count != 0)
      insert(this->end(), val, val);
    else
      erase(I, this->end());
  }
  template<class InIt> void assign(InIt first1, InIt last1) {
    iterator first2 = this->begin(), last2 = this->end();
    for ( ; first1 != last1 && first2 != last2; ++first1, ++first2)
      *first1 = *first2;
    if (first2 == last2)
      erase(first1, last1);
    else
      insert(last1, first2, last2);
  }


  // Resize members...
  void resize(size_type newsize, NodeTy val) {
    iterator i = this->begin();
    size_type len = 0;
    for ( ; i != this->end() && len < newsize; ++i, ++len) /* empty*/ ;

    if (len == newsize)
      erase(i, this->end());
    else                                          // i == end()
      insert(this->end(), newsize - len, val);
  }
  void resize(size_type newsize) { resize(newsize, NodeTy()); }
};

} // End llvm namespace

namespace std {
  // Ensure that swap uses the fast list swap...
  template<class Ty>
  void swap(llvm::iplist<Ty> &Left, llvm::iplist<Ty> &Right) {
    Left.swap(Right);
  }
}  // End 'std' extensions...

#endif // LLVM_ADT_ILIST_H
