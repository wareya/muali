#ifndef _INCLUDE_BXX_ROPE
#define _INCLUDE_BXX_ROPE

#include <cstddef> // ptrdiff_t
#include <cstdint> // int types
#include <cstdio> // puts, printf
#include <utility> // std::move

// min_size must be less than half of max_size and ideally should significantly less, e.g. 1/4 or 1/8 of max_size seem to be good.
// text editing seems to prefer small max_size values like 192, sorting seems to prefer large values like 512.
// split_left should be relatively close to half of max_size. larger than half causes right-edge insertion to be slightly faster and vice versa.
template<typename T, int min_size = 32, int max_size = 256, int split_left = 192>
class Rope {
private:
    class RopeNode;
    
    class alignas(alignof(T) > alignof(size_t) ? alignof(T) : alignof(size_t))
    RopeNodeLeaf : public RopeNode {
        char mbuffer[max_size * sizeof(T)];
        
    public:
        
        ~RopeNodeLeaf()
        {
            for (size_t i = 0; i < this->mlength; i++)
                ((T*)mbuffer)[i].~T();
        }
        
        T & item(size_t i)
        {
            return ((T*)mbuffer)[i];
        }
        
        const T & item(size_t i) const
        {
            return ((const T*)mbuffer)[i];
        }
        
        template <typename U>
        void leaf_insert_at(size_t i, U && item)
        {
            if (this->mlength + 1 > max_size || i >= max_size)
                throw;
            
            this->mlength += 1;
            
            try
            {
                for (size_t j = this->mlength - 1; j > i; j--)
                {
                    ::new((void*)(((T*)mbuffer) + j)) T(std::move(((T*)mbuffer)[j - 1]));
                    ((T*)mbuffer)[j - 1].~T();
                }
                ::new((void*)(((T*)mbuffer) + i)) T(std::forward<U>(item));
            }
            catch (...)
            {
                this->mlength -= 1;
                throw;
            }
        }
        
        T leaf_erase_at(size_t i)
        {
            if (i >= this->mlength || this->mlength == 0)
                throw;
            
            T ret(std::move(((T*)mbuffer)[i]));
            
            for (size_t j = i; j + 1 < this->mlength; j++)
            {
                ((T*)mbuffer)[j].~T();
                ::new((void*)(((T*)mbuffer) + j)) T(std::move(((T*)mbuffer)[j + 1]));
            }
            
            ((T*)mbuffer)[this->mlength - 1].~T();
            this->mlength -= 1;
            
            return ret;
        }
        void steal_data(RopeNodeLeaf * other, size_t start, size_t end)
        {
            T * buf = (T*)(other->mbuffer);
            for (size_t i = start; i < end; i++)
            {
                ::new((void*)(((T*)mbuffer) + this->mlength)) T(std::move(buf[i]));
                buf[i].~T();
                
                this->mlength += 1;
            }
        }
        void push_back(T && item)
        {
            Rope::insert_at(this, this->size(), item);
        }
        void push_back(const T & item)
        {
            Rope::insert_at(this, this->size(), item);
        }
    };
    
    class RopeNode {
    public:
        size_t mlength = 0;
        size_t mheight = 0;
        size_t mleaves = 1;
        
        RopeNode * left = 0;
        RopeNode * right = 0;
        RopeNode * parent = 0;
        
        ~RopeNode()
        {
            if (left)
            {
                if (left->left)
                    delete left;
                else
                    delete static_cast<RopeNodeLeaf *>(left);
            }
            if (right)
            {
                if (right->left)
                    delete right;
                else
                    delete static_cast<RopeNodeLeaf *>(right);
            }
        }
        
        template<typename D>
        static RopeNode * from_copy(const D & data, size_t start, size_t count)
        {
            if (count <= max_size)
            {
                auto ret = new RopeNodeLeaf();
                for (size_t i = 0; i < count; i++)
                    static_cast<RopeNodeLeaf *>(ret)->push_back(data[start + i]);
                
                ret->mlength = count;
                return ret;
            }
            auto ret = new RopeNode();
            ret->left = from_copy(data, start, count / 2);
            ret->right = from_copy(data, start + count / 2, count - count / 2);
            ret->left->parent = ret;
            ret->right->parent = ret;
            ret->fix_metadata();
            return ret;
        }
        
        template<typename D>
        static RopeNode * from_move(const D & data, size_t start, size_t count)
        {
            if (count <= max_size)
            {
                auto ret = new RopeNodeLeaf();
                for (size_t i = 0; i < count; i++)
                    static_cast<RopeNodeLeaf *>(ret)->push_back(std::move(data[start + i]));
                
                ret->mlength = count;
                return ret;
            }
            auto ret = new RopeNode();
            ret->left = from_move(data, start, count / 2);
            ret->right = from_move(data, start + count / 2, count - count / 2);
            ret->left->parent = ret;
            ret->right->parent = ret;
            ret->fix_metadata();
            return ret;
        }
        
        size_t size() const
        {
            return mlength;
        }
        const T & get_at(size_t i) const
        {
            if (!left && i < static_cast<const RopeNodeLeaf *>(this)->size())
                return static_cast<const RopeNodeLeaf *>(this)->item(i);
            else if (i < left->size())
                return left->get_at(i);
            else
                return right->get_at(i - left->size());
        }
        T & get_at(size_t i)
        {
            if (!left && i < static_cast<RopeNodeLeaf *>(this)->size())
                return static_cast<RopeNodeLeaf *>(this)->item(i);
            else if (i < left->size())
                return left->get_at(i);
            else
                return right->get_at(i - left->size());
        }
        const T & operator[](size_t pos) const { return get_at(pos); }
        T & operator[](size_t pos) { return get_at(pos); }
        
        void print_structure(size_t depth) const
        {
            for (size_t i = 0; i < depth; i++)
                printf("-");
            if (left)
            {
                printf("branch\n");
                left->print_structure(depth + 1);
                right->print_structure(depth + 1);
                return;
            }
            printf("leaf: `");
            for (size_t i = 0; i < static_cast<RopeNodeLeaf *>(this)->items.size(); i++)
                printf("%c", static_cast<RopeNodeLeaf *>(this)->items[i]);
            puts("`");
        }
        void fix_metadata()
        {
            if (left)
            {
                if (!right) throw;
                mheight = left->mheight + 1;
                if (mheight < right->mheight + 1)
                    mheight = right->mheight + 1;
                mlength = left->mlength + right->mlength;
                mleaves = left->mleaves + right->mleaves;
            }
            else
            {
                if (right) throw;
                mheight = 0;
                mleaves = 1;
            }
        }
    };
    
    static void rebalance_impl_bottom_up(RopeNode * node)
    {
        if (!node)
            return;
        
        if (node->mheight <= 2)
        {
            if (node->parent)
                rebalance_impl_bottom_up(node->parent);
            return;
        }
        ptrdiff_t height_diff = node->left->mheight - node->right->mheight;
        
        if (height_diff < 0)
            height_diff = -height_diff;
        
        if (height_diff == 0)
            return;
        
        if (height_diff > 1)
        {
            if (node->left->mheight > node->right->mheight)
            {
                auto temp = node->right;
                
                node->right = node->left;
                node->left = node->right->left;
                node->right->left = node->right->right;
                node->right->right = temp;
                
                node->left->parent = node;
                node->right->right->parent = node->right;
                node->right->fix_metadata();
            }
            else
            {
                auto temp = node->left;
                
                node->left = node->right;
                node->right = node->left->right;
                node->left->right = node->left->left;
                node->left->left = temp;
                
                node->right->parent = node;
                node->left->left->parent = node->left;
                node->left->fix_metadata();
            }
            
            node->fix_metadata();
        }
        
        rebalance_impl_bottom_up(node->parent);
    }
    
    T erase_at(RopeNode * node, size_t i)
    {
        while (node->left)
        {
            if (i < node->left->size())
                node = node->left;
            else
            {
                i -= node->left->size();
                node = node->right;
            }
        }
        
        if (!(i < static_cast<RopeNodeLeaf *>(node)->size())) throw;
        T item(static_cast<RopeNodeLeaf *>(node)->leaf_erase_at(i));
        
        if (node->mlength < min_size && node->parent && (node->parent->mlength - 1 < min_size || node->mlength == 0))
        {
            size_t oldlen = node->mlength;
            auto rnode = node;
            
            while (rnode->parent && (rnode->parent->mlength - 1 < min_size || rnode->mlength == 0))
            {
                rnode = rnode->parent;
                rnode->mlength -= 1;
            }
            
            RopeNode * next;
            if (oldlen == 0)
            {
                if (rnode->left->mlength == 0)
                {
                    next = rnode->right;
                    delete rnode->left;
                }
                else
                {
                    next = rnode->left;
                    delete rnode->right;
                }
                
            }
            else
            {
                next = new RopeNodeLeaf();
                for (size_t i = 0; i < rnode->mlength; i++)
                    static_cast<RopeNodeLeaf *>(next)->push_back(std::move(rnode->get_at(i)));
            }
            
            if (rnode->parent)
            {
                if (rnode->parent->left == rnode)
                    rnode->parent->left = next;
                else
                    rnode->parent->right = next;
            }
            next->parent = rnode->parent;
            
            if (oldlen == 0)
            {
                rnode->left = nullptr;
                rnode->right = nullptr;
                rnode->parent = nullptr;
            }
            
            if (rnode == root)
                root = next;
            delete rnode;
            rnode = next;
            
            while (rnode->parent)
            {
                rnode = rnode->parent;
                
                rnode->mheight = rnode->left->mheight + 1;
                if (rnode->mheight < rnode->right->mheight + 1)
                    rnode->mheight = rnode->right->mheight + 1;
                rnode->mleaves = rnode->left->mleaves + rnode->right->mleaves;
                
                rnode->mlength -= 1;
            }
            
            rebalance_impl_bottom_up(next);
        }
        else
        {
            auto rnode = node;
            while (rnode->parent)
            {
                rnode = rnode->parent;
                rnode->mlength -= 1;
            }
        }
        
        return item;
    }
    
    static void insert_at(RopeNode * node, size_t i, T item)
    {
        while (node->left)
        {
            if (i <= node->left->size())
                node = node->left;
            else
            {
                i -= node->left->size();
                node = node->right;
            }
        }
        
        if (!(i <= node->size())) throw;
        static_cast<RopeNodeLeaf *>(node)->leaf_insert_at(i, item);
        
        if (node->mlength >= max_size)
        {
            auto newnode = new RopeNode();
            
            if (node->parent)
            {
                if (node->parent->left == node)
                    node->parent->left = newnode;
                else
                    node->parent->right = newnode;
            }
            
            newnode->parent = node->parent;
            
            newnode->left = node;
            newnode->left->parent = newnode;
            
            newnode->right = new RopeNodeLeaf();
            newnode->right->parent = newnode;
            
            static_cast<RopeNodeLeaf *>(newnode->right)->steal_data(
                //static_cast<RopeNodeLeaf *>(newnode->left), newnode->left->mlength / 2, newnode->left->mlength);
                static_cast<RopeNodeLeaf *>(newnode->left), split_left, newnode->left->mlength);
            
            newnode->mlength = newnode->left->mlength;
            newnode->left->mlength -= newnode->right->mlength;
            
            if(!(newnode->mlength == newnode->left->mlength + newnode->right->mlength)) throw;
            
            newnode->mheight = 1;
            newnode->mleaves += 1;
            
            auto rnode = newnode;
            auto prev = rnode;
            rnode = rnode->parent;
            
            while (rnode)
            {
                if (rnode->mheight < prev->mheight + 1)
                    rnode->mheight = prev->mheight + 1;
                
                rnode->mleaves += 1;
                rnode->mlength += 1;
                
                prev = rnode;
                rnode = rnode->parent;
            }
            
            rebalance_impl_bottom_up(newnode);
        }
        else
        {
            auto rnode = node->parent;
            while (rnode)
            {
                rnode->mlength += 1;
                rnode = rnode->parent;
            }
        }
    }
    template <ptrdiff_t dir = 1>
    class RopeIterator {
        Rope * rope;
        ptrdiff_t index = 0;
        
        ptrdiff_t wrap_index(ptrdiff_t i) const
        {
            i *= dir;
            if (index + i < 0)
                return index + i + 1 + (ptrdiff_t)rope->size();
            return index + i;
        }
        
    public:
        
#ifdef BXX_STDLIB_ITERATOR_INCLUDED
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = T *;
        using reference = T &;
        using iterator_category = std::random_access_iterator_tag;
#endif // BXX_STDLIB_ITERATOR_INCLUDED
        
        RopeIterator(Rope * rope, ptrdiff_t index) : rope(rope), index(index) { }
        
        const T & operator[](ptrdiff_t pos) const noexcept { return (*rope)[wrap_index(pos)]; }
        T & operator[](ptrdiff_t pos) noexcept { return (*rope)[wrap_index(pos)]; }
        const T & operator*() const { return (*rope)[wrap_index(0)]; }
        T & operator*() { return (*rope)[wrap_index(0)]; }
        const T * operator->() const noexcept { return &(*rope)[wrap_index(0)]; }
        T * operator->() noexcept { return &(*rope)[wrap_index(0)]; }
        
        RopeIterator & operator++() { index += dir; return *this; }
        RopeIterator & operator--() { index -= dir; return *this; }
        
        RopeIterator & operator++(int) { auto ret = *this; index += dir; return ret; }
        RopeIterator & operator--(int) { auto ret = *this; index -= dir; return ret; }
        
        RopeIterator & operator+=(ptrdiff_t n)
        {
            index += dir * n;
            return *this;
        }
        RopeIterator & operator-=(ptrdiff_t n)
        {
            index -= dir * n;
            return *this;
        }
        
        bool operator> (const RopeIterator & other) const { return wrap_index(0) >  other.wrap_index(0); }
        bool operator>=(const RopeIterator & other) const { return wrap_index(0) >= other.wrap_index(0); }
        bool operator< (const RopeIterator & other) const { return wrap_index(0) <  other.wrap_index(0); }
        bool operator<=(const RopeIterator & other) const { return wrap_index(0) <= other.wrap_index(0); }
        bool operator!=(const RopeIterator & other) const { return wrap_index(0) != other.wrap_index(0); }
        bool operator==(const RopeIterator & other) const { return wrap_index(0) == other.wrap_index(0); }
        
        friend RopeIterator operator+(ptrdiff_t n, RopeIterator it)
        {
            it.index += dir * n;
            return it;
        }
        friend RopeIterator operator+(RopeIterator it, ptrdiff_t n)
        {
            it.index += dir * n;
            return it;
        }
        friend RopeIterator operator-(ptrdiff_t n, RopeIterator it)
        {
            it.index -= dir * n;
            return it;
        }
        friend RopeIterator operator-(RopeIterator it, ptrdiff_t n)
        {
            it.index -= dir * n;
            return it;
        }
        
        ptrdiff_t operator-(const RopeIterator & other) const
        {
            return wrap_index(index) - wrap_index(other.index);
        }
    };
    
    RopeNode * root = 0;
    
    size_t cached_node_start = 0;
    size_t cached_node_len = 0;
    RopeNode * cached_node = 0;
    
    void kill_cache()
    {
        cached_node = 0;
        cached_node_len = 0;
        cached_node_start = 0;
    }
    
    void fix_cache(size_t prev_leaves, size_t i, ptrdiff_t diff)
    {
        if (!cached_node)
            return;
        
        if (prev_leaves != root->mleaves) // structure changed. kill cache.
            kill_cache();
        
        else if (diff > 0)
        {
            if (i < cached_node_start)
                cached_node_start += diff;
            else if (i <= cached_node_start + cached_node_len)
                cached_node_len += diff;
        }
        else
        {
            // erase ran into cached node. kill cache.
            if (i - diff >= cached_node_start)
                kill_cache();
            else
                cached_node_start -= diff;
        }
    }
    
    void kill_root()
    {
        if (root)
        {
            if (root->left)
                delete root;
            else
                delete static_cast<RopeNodeLeaf *>(root);
        }
    }
    
public:
    
    void clear()
    {
        kill_root();
        kill_cache();
        root = 0;
    }
    
    Rope() = default;
    Rope(Rope &&) = default;
    Rope(const Rope & other)
    {
        if (other.root)
            root = RopeNodeLeaf::from_copy(*other.root, 0, other.size());
    }
    
    ~Rope()
    {
        kill_root();
        root = 0;
    }
    
    Rope & operator=(Rope && other) = default;
    Rope & operator=(const Rope & other)
    {
        if (root) kill_root();
        
        root = RopeNodeLeaf::from_copy(*other.root, 0, other.size());
        kill_cache();
        return *this;
    }
    
    T & operator[](size_t pos)
    {
        if (!root) throw;
        
        if (cached_node && pos >= cached_node_start && pos < cached_node_start + cached_node_len)
            return (*cached_node)[pos - cached_node_start];
        
        cached_node_start = 0;
        cached_node = root;
        size_t i = pos;
        while (cached_node->left)
        {
            if (i < cached_node->left->size())
                cached_node = cached_node->left;
            else
            {
                i -= cached_node->left->size();
                cached_node_start += cached_node->left->size();
                cached_node = cached_node->right;
            }
        }
        cached_node_len = cached_node->size();
        
        return (*cached_node)[i];
    }
    
    size_t size() const
    {
        if (root)
            return root->mlength;
        return 0;
    }
    void insert_at(size_t i, T item, bool update_cache = false)
    {
        if (!root)
        {
            root = new RopeNodeLeaf();
            kill_cache();
        }
        if (i > size()) throw;
        
        size_t prev_leaves = root->mleaves;
        
        if (cached_node && i >= cached_node_start && i <= cached_node_start + cached_node_len)
            insert_at(cached_node, i - cached_node_start, item);
        else
            insert_at(root, i, item);
        
        while (root->parent)
            root = root->parent;
        
        fix_cache(prev_leaves, i, 1);
        
        if (update_cache)
            (*this)[i];
    }
    void insert(size_t i, T item)
    {
        insert_at(i, item);
    }
    void push_back(T item)
    {
        insert_at(size(), item);
    }
    T erase_at(size_t i, bool update_cache = false)
    {
        if (!root)
            throw;
        
        if (i >= size()) throw;
        
        size_t prev_leaves = root->mleaves;
        
        if (update_cache)
            (*this)[i];
        
        T ret = [&]()
        {
            if (cached_node && i >= cached_node_start && i < cached_node_start + cached_node_len)
                return erase_at(cached_node, i - cached_node_start);
            return erase_at(root, i);
        }();
        
        while (root->parent)
            root = root->parent;
        
        fix_cache(prev_leaves, i, -1);
        return ret;
    }
    void erase(size_t i)
    {
        erase_at(i);
    }
    void erase(size_t i, size_t count)
    {
        for (size_t n = 0; n < count; n++)
            erase_at(i);
    }
    void print_structure() const
    {
        if (!root)
            return (void)puts("empty");
        root->print_structure(0);
    }
    
#ifdef _INCLUDE_BXX_SORTING
    /// Sorts the rope.
    /// Uses insertion sort, which, for ropes, is O(n logn logn) in the worst case and O(n logn) in the best case.
    /// Insertion sort is the ideal sorting algorithm for ropes.
    /// The given f is a function that returns 1 for less-than and 0 otherwise.
    template<typename Comparator>
    void sort(Comparator f)
    {
        inout_tree_insertion_sort_impl<T>(f, *this, 0, size());
    }
#endif
    
    RopeIterator<1> begin()
    {
        return RopeIterator<1>{this, 0};
    }
    RopeIterator<1> end()
    {
        return RopeIterator<1>{this, -1};
    }
};
#endif // _INCLUDE_BXX_Rope
