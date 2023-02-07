#ifndef HASH_SPAN_H_
#define HASH_SPAN_H_

#include <memory>
#include <utility>

// SetSpan is a type-erased version of a hash table, useful for example, for
// writing tests that should work on any hash table implementation.

template <class Key>
class Iterator {
 public:
  template <class IteratorT>
  Iterator(IteratorT&& iterator)
      : iterator_(std::make_shared<Model<IteratorT>>(std::forward<IteratorT>(iterator))) {}
 private:
  class Concept {
  };
  template <class IteratorT>
  class Model : public Concept {
   public:
    Model(const IteratorT& iterator) : iterator_(iterator) {}
   private:
    IteratorT iterator_;
  };
  std::shared_ptr<Concept> iterator_;
};

template <class Key>
class SetSpan {
 public:
  using value_type = Key;

  template <class Table>
  SetSpan(Table&& table)
      : table_(std::make_shared<Model<Table>>(std::forward<Table>(table))) {}

  std::pair<Iterator<Key>, bool> insert(const Key& key) {
    return table_->insert(key);
  }

  size_t size() const { return table_->size(); }

 private:
  class Concept {
   public:
    virtual size_t size() const = 0;
    virtual std::pair<Iterator<Key>, bool> insert(const Key& key) = 0;
  };
  template <class Table>
  class Model : public Concept {
   public:
    Model(const Table& table) : table_(table) {}
    size_t size() const { return table_.size(); }
    std::pair<Iterator<Key>, bool> insert(const Key& key) {
      auto [it, inserted] = table_.insert(key);
      return {Iterator<Key>(it), inserted};
    }

   private:
    Table table_;
  };
  std::shared_ptr<Concept> table_;
};

#if 0
template <class Key>
class SetSpan {
 public:
#if 0
  using key_type = Key;
#endif
  using value_type = Key;

#if 0
  template <class Table>
  SetSpan(Table&& table)
      : table_(std::make_shared<Model<Table>>(std::forward<Table>(table))) {}
#endif

  std::pair<iterator, bool> insert(const value_type& value) {
    return iterator(table_->insert(value));
  }

  class iterator {
  };
#if 0
  class iterator {
   public:
    template <class Iterator>
    Iterator(Iterator&& it)
        : it_(std::make_shared <
              IteratorModel<Iterator>(std::forward<Iterator>(it))) {}
  };
#endif

 private:
  class Concept {
    // TODO: Why fully qualified Iterator?
    virtual std::pair<typename SetSpan<Key>::Iterator, bool> insert(
        const value_type& value) = 0;
  };

#if 0
  template <class Table>
  class Model : Concept {};
#endif

  std::shared_ptr<Concept> table_;
};
#endif

#endif  // HASH_SPAN_H_
