#pragma once

#include <type_traits>

#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>
#include <utility>

template< typename T >
class BucketStorage
{
  public:
	using size_type = std::size_t;
	using value_type = T;
	using reference = T &;
	using const_reference = const T &;
	using pointer = T *;
	using const_pointer = const T *;
	using difference_type = std::ptrdiff_t;

	template< bool isConst >
	class abstract_iterator;
	using iterator = abstract_iterator< false >;
	using const_iterator = abstract_iterator< true >;

	explicit BucketStorage(size_type block_capacity = 64);
	BucketStorage(const BucketStorage &bs);
	BucketStorage(BucketStorage &&bs) noexcept;
	BucketStorage &operator=(const BucketStorage &other);
	BucketStorage &operator=(BucketStorage &&other) noexcept;
	~BucketStorage() noexcept;

	iterator insert(const value_type &value);
	iterator insert(value_type &&value);
	iterator erase(const_iterator it) noexcept;
	bool empty() const noexcept;
	size_type size() const noexcept;
	size_type capacity() const noexcept;
	void shrink_to_fit();
	void clear() noexcept;
	void swap(BucketStorage &other) noexcept;
	iterator end() noexcept;
	const_iterator end() const noexcept;
	const_iterator cend() const noexcept;
	iterator begin() noexcept;
	const_iterator begin() const noexcept;
	const_iterator cbegin() const noexcept;
	iterator get_to_distance(iterator it, const difference_type distance) noexcept;

  private:
	template< typename S >
	class Stack;
	class Bucket;
	class Node;
	class BlockNode;
	class LinkedStack;

	size_type sz_;
	size_type cap_;
	size_type block_capacity_;
	size_type time_;
	Node *last_;
	Node *first_;
	Bucket *last_block_;
	Bucket *first_block_;
	LinkedStack *rows_;

	void resize();
	template< typename S >
	iterator insert_element(S &&value);
};

template< typename T >
BucketStorage< T >::BucketStorage(size_type block_capacity) :
	sz_(0), cap_(0), block_capacity_(block_capacity), time_(0), last_(new Node()), first_(nullptr),
	last_block_(nullptr), first_block_(nullptr), rows_(nullptr)
{
}

template< typename T >
BucketStorage< T >::BucketStorage(const BucketStorage &bs) :
	sz_(0), cap_(0), block_capacity_(bs.block_capacity_), time_(0), last_(new Node()), first_(nullptr),
	last_block_(nullptr), first_block_(nullptr), rows_(nullptr)
{
	for (const_iterator i = bs.begin(); i < bs.end(); i++)
		insert(*i);
}

template< typename T >
BucketStorage< T >::BucketStorage(BucketStorage &&bs) noexcept :
	sz_(bs.sz_), cap_(bs.cap_), block_capacity_(bs.block_capacity_), time_(bs.time_), last_(bs.last_),
	first_(bs.first_), last_block_(bs.last_block_), first_block_(bs.first_block_), rows_(bs.rows_)
{
	bs.sz_ = 0;
	bs.cap_ = 0;
	bs.time_ = 0;
	bs.block_capacity_ = 0;
	bs.first_block_ = nullptr;
	bs.last_block_ = nullptr;
	bs.first_ = nullptr;
	bs.last_ = nullptr;
	bs.rows_ = nullptr;
}

template< typename T >
BucketStorage< T > &BucketStorage< T >::operator=(const BucketStorage &other)
{
	if (this != &other)
		BucketStorage< T >(other).swap(*this);
	return *this;
}

template< typename T >
BucketStorage< T > &BucketStorage< T >::operator=(BucketStorage &&other) noexcept
{
	if (this != &other)
		BucketStorage< T >(std::move(other)).swap(*this);
	return *this;
}

template< typename T >
template< bool isConst >
class BucketStorage< T >::abstract_iterator
{
  public:
	using difference_type = std::ptrdiff_t;
	using value_type = std::conditional_t< isConst, const T, T >;
	using node_type = std::conditional_t< isConst, const Node, Node >;
	using cond_reference = std::conditional_t< isConst, const_reference, reference >;
	using cond_pointer = std::conditional_t< isConst, const_pointer, pointer >;

	explicit abstract_iterator(node_type *ptr = nullptr) noexcept : node_(ptr) {}

	abstract_iterator< isConst > operator++(int) noexcept
	{
		abstract_iterator< isConst > temp = *this;
		++*this;
		return temp;
	}

	abstract_iterator< isConst > &operator++() noexcept
	{
		node_ = node_->next_;
		return *this;
	}

	abstract_iterator< isConst > &operator--() noexcept
	{
		node_ = node_->prev_;
		return *this;
	}

	abstract_iterator< isConst > operator--(int) noexcept
	{
		abstract_iterator< isConst > temp = *this;
		--*this;
		return temp;
	}

	bool operator==(const abstract_iterator< isConst > &it) const noexcept
	{
		return (!node_ && !it.node_) || (!node_ && it.node_ && it.node_->time_ == std::numeric_limits< size_type >::max()) ||
			   (node_ && !it.node_ && node_->time_ == std::numeric_limits< size_type >::max()) ||
			   (node_ && it.node_ && node_ == it.node_);
	}

	bool operator!=(abstract_iterator< true > &it) const noexcept { return !(*this == it); }
	bool operator<(const abstract_iterator< isConst > &it) const noexcept { return node_->time_ < it.node_->time_; }
	bool operator<=(const abstract_iterator< isConst > &it) const noexcept { return *this == it || *this < it; }
	bool operator>(const abstract_iterator< isConst > &it) const noexcept { return node_->time_ > it.node_->time_; }
	bool operator>=(const abstract_iterator< isConst > &it) const noexcept { return *this == it || *this > it; }
	operator const abstract_iterator< true >() const noexcept { return abstract_iterator< true >(node_); }
	cond_reference operator*() const noexcept { return *(node_->ptr_); }
	cond_pointer operator->() const noexcept { return node_->ptr_; }
	node_type *getNode() const noexcept { return node_; }

	void erase() noexcept
	{
		if (node_->next_)
			node_->next_->prev_ = node_->prev_;
		if (node_->prev_)
			node_->prev_->next_ = node_->next_;
		delete node_;
	}

  private:
	node_type *node_;
};

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::insert(const value_type &value)
{
	return insert_element(value);
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::insert(value_type &&value)
{
	return insert_element(std::move(value));
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::erase(const_iterator it) noexcept
{
	iterator next = iterator{ it.getNode()->next_ };
	if (begin() == it)
		first_ = first_->next_;
	Bucket *row = it.getNode()->row_;
	if (!row->has_free_places())
		rows_->push(row);
	row->del(it.getNode()->col_);
	it.erase();
	if (row->is_empty())
	{
		rows_->remove(row);
		cap_ -= block_capacity_;
		if (row == first_block_)
			first_block_ = first_block_->next_;
		if (row == last_block_)
			last_block_ = last_block_->prev_;
		if (row->next_)
			row->next_->prev_ = row->prev_;
		if (row->prev_)
			row->prev_->next_ = row->next_;
		delete row;
	}
	--sz_;
	return next;
}

template< typename T >
bool BucketStorage< T >::empty() const noexcept
{
	return sz_ == 0;
}

template< typename T >
typename BucketStorage< T >::size_type BucketStorage< T >::size() const noexcept
{
	return sz_;
}

template< typename T >
typename BucketStorage< T >::size_type BucketStorage< T >::capacity() const noexcept
{
	return cap_;
}

template< typename T >
void BucketStorage< T >::shrink_to_fit()
{
	BucketStorage bs(block_capacity_);
	bs.swap(*this);
	for (iterator i = bs.begin(); i < bs.end(); i++)
		insert(std::move(*i));
}

template< typename T >
void BucketStorage< T >::clear() noexcept
{
	for (size_type i = sz_; i > 0; i--)
		erase(begin());
}

template< typename T >
void BucketStorage< T >::swap(BucketStorage &other) noexcept
{
	std::swap(sz_, other.sz_);
	std::swap(cap_, other.cap_);
	std::swap(block_capacity_, other.block_capacity_);
	std::swap(time_, other.time_);
	std::swap(last_, other.last_);
	std::swap(first_, other.first_);
	std::swap(last_block_, other.last_block_);
	std::swap(first_block_, other.first_block_);
	std::swap(rows_, other.rows_);
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::end() noexcept
{
	return iterator{ last_ };
}

template< typename T >
typename BucketStorage< T >::const_iterator BucketStorage< T >::end() const noexcept
{
	return const_iterator{ last_ };
}

template< typename T >
typename BucketStorage< T >::const_iterator BucketStorage< T >::cend() const noexcept
{
	return const_iterator{ last_ };
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::begin() noexcept
{
	return iterator{ first_ };
}

template< typename T >
typename BucketStorage< T >::const_iterator BucketStorage< T >::begin() const noexcept
{
	return const_iterator{ first_ };
}

template< typename T >
typename BucketStorage< T >::const_iterator BucketStorage< T >::cbegin() const noexcept
{
	return const_iterator{ first_ };
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::get_to_distance(iterator it, const difference_type distance) noexcept
{
	std::advance(it, distance);
	return it;
}

template< typename T >
BucketStorage< T >::~BucketStorage() noexcept
{
	clear();
	delete last_;
	while (first_block_)
	{
		last_block_ = first_block_->next_;
		delete first_block_;
		first_block_ = last_block_;
	}
	delete rows_;
}

template< typename T >
template< typename S >
typename BucketStorage< T >::iterator BucketStorage< T >::insert_element(S &&value)
{
	resize();
	Bucket *row = rows_->pop();
	Node *newNode = row->insert(std::forward< S >(value), time_);
	if (last_->prev_)
		last_->prev_->next_ = newNode;
	newNode->prev_ = last_->prev_;
	newNode->next_ = last_;
	last_->prev_ = newNode;
	time_++;
	if (sz_ == 0)
		first_ = newNode;
	sz_++;
	if (row->has_free_places())
		rows_->push(row);
	return iterator{ newNode };
}

template< typename T >
void BucketStorage< T >::resize()
{
	if (time_ == 0)
	{
		first_block_ = new Bucket(block_capacity_);
		last_block_ = first_block_;
		rows_ = new LinkedStack();
		rows_->push(first_block_);
		cap_ = block_capacity_;
	}
	if (!rows_->is_empty())
		return;
	last_block_->next_ = new Bucket(block_capacity_);
	last_block_->next_->prev_ = last_block_;
	last_block_ = last_block_->next_;
	rows_->push(last_block_);
	cap_ += block_capacity_;
}

template< typename T >
class BucketStorage< T >::Node
{
  public:
	Node(T *ptr = nullptr,
		 Node *prev = nullptr,
		 Node *next = nullptr,
		 Bucket *row = nullptr,
		 size_type col = std::numeric_limits< size_type >::max(),
		 size_type time = std::numeric_limits< size_type >::max()) noexcept :
		ptr_(ptr), prev_(prev), next_(next), row_(row), col_(col), time_(time)
	{
	}

  private:
	friend class BucketStorage;
	pointer ptr_;
	Node *prev_;
	Node *next_;
	Bucket *row_;
	size_type col_;
	size_type time_;
};

template< typename T >
template< typename S >
class BucketStorage< T >::Stack
{
  public:
	Stack() : arr_(static_cast< S * >(::operator new(sizeof(S)))), arr_sz_(0), cap_(1) {}

	~Stack() noexcept { clear(); }

	S pop() noexcept
	{
		S res(std::move(arr_[--arr_sz_]));
		(arr_ + arr_sz_)->~S();
		return res;
	}

	void push(const S &value) { push_back(value); }
	void push(S &&value) { push_back(std::move(value)); }

  private:
	S *arr_;
	size_type arr_sz_;
	size_type cap_;

	void clear() noexcept
	{
		for (size_type i = 0; i < arr_sz_; i++)
			(arr_ + i)->~S();
		::operator delete(arr_);
	}

	template< typename H >
	void push_back(H &&value)
	{
		if (arr_sz_ >= cap_)
			resize();
		new (arr_ + arr_sz_) S(std::forward< H >(value));
		arr_sz_++;
	}

	void resize()
	{
		S *newArr = static_cast< S * >(::operator new((cap_ << 1) * sizeof(S)));
		try
		{
			std::uninitialized_move(arr_, arr_ + arr_sz_, newArr);
		} catch (...)
		{
			::operator delete(newArr);
			throw;
		}
		clear();
		arr_ = newArr;
		cap_ <<= 1;
	}
};

template< typename T >
class BucketStorage< T >::Bucket
{
  public:
	explicit Bucket(size_type block_capacity) :
		next_(nullptr), prev_(nullptr), node_(nullptr), capacity_(block_capacity), size_(0), st_(new Stack< size_type >())
	{
		try
		{
			block_ = static_cast< T * >(::operator new(block_capacity * sizeof(T)));
		} catch (...)
		{
			delete st_;
			throw;
		}
		for (size_type i = 0; i < block_capacity; i++)
			st_->push(block_capacity - i - 1);
	}

	~Bucket() noexcept
	{
		::operator delete(block_);
		delete st_;
	}

	Node *insert(const value_type &value, size_type t) { return insert_element(value, t); }
	Node *insert(value_type &&value, size_type t) { return insert_element(std::move(value), t); }
	bool has_free_places() const noexcept { return size_ < capacity_; }
	bool is_empty() const noexcept { return size_ == 0; }

	void del(size_type ind)
	{
		(block_ + ind)->~T();
		st_->push(ind);
		--size_;
	}

	void erase() noexcept
	{
		if (node_->next_)
			node_->next_->prev_ = node_->prev_;
		if (node_->prev_)
			node_->prev_->next_ = node_->next_;
		delete node_;
	}

  private:
	friend class BucketStorage;
	Bucket *next_;
	Bucket *prev_;
	BlockNode *node_;
	T *block_;
	size_type capacity_;
	size_type size_;
	Stack< size_type > *st_;

	template< typename S >
	Node *insert_element(S &&value, size_type t)
	{
		size_type ind = st_->pop();
		try
		{
			new (block_ + ind) T(std::forward< S >(value));
		} catch (...)
		{
			st_->push(ind);
			throw;
		}
		size_++;
		return new Node(block_ + ind, nullptr, nullptr, this, ind, t);
	}
};

template< typename T >
class BucketStorage< T >::BlockNode
{
  public:
	BlockNode(Bucket *block = nullptr, BlockNode *prev = nullptr, BlockNode *next = nullptr) noexcept :
		block_(block), prev_(prev), next_(next)
	{
	}

  private:
	friend class BucketStorage;
	Bucket *block_;
	BlockNode *prev_;
	BlockNode *next_;
};

template< typename T >
class BucketStorage< T >::LinkedStack
{
  public:
	LinkedStack() noexcept : first_(nullptr), last_(nullptr), size_(0) {}

	~LinkedStack() noexcept
	{
		for (size_type i = size_; i > 0; i--)
		{
			last_ = first_->next_;
			delete first_;
			first_ = last_;
		}
	}

	void push(Bucket *block)
	{
		size_++;
		if (!first_)
		{
			first_ = new BlockNode(block, nullptr, nullptr);
			last_ = first_;
		}
		else
		{
			last_->next_ = new BlockNode(block, last_, nullptr);
			last_ = last_->next_;
		}
		block->node_ = last_;
	}

	void remove(Bucket *block) noexcept
	{
		if (first_ == block->node_)
			first_ = first_->next_;
		if (last_ == block->node_)
			last_ = last_->prev_;
		block->erase();
		--size_;
	}

	Bucket *pop() noexcept
	{
		Bucket *res = last_->block_;
		remove(res);
		return res;
	}

	bool is_empty() const noexcept { return size_ == 0; }

  private:
	BlockNode *first_;
	BlockNode *last_;
	size_type size_;
};
