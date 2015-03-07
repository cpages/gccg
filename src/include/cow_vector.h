/*
    Copy-on-write vectors implemented using reference count.
    Copyright (C) 2003,2004 Tommi Ronkainen

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program, in the file license.txt. If not, write
  to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA.
*/

#include <vector>

// #define COW_DEBUG 1
// #define COW_DUMP_MALLOCS 1

template<class T> class cow_vector;

template<class T> class cow_data
{
	friend class cow_vector<T>;

#if COW_DEBUG
	static int countmalloc;
#endif
	
	unsigned refcount;
	std::vector<T>* data;

	/// Create empty data link.
	cow_data()
	{
#if COW_DEBUG
		countmalloc++;
#endif
		refcount=1;
		data=new std::vector<T>();
	}

	/// Create data link initialized using the given data.
	cow_data(std::vector<T>* v)
	{
#if COW_DEBUG
		countmalloc++;
#endif
		refcount=1;
		data=new std::vector<T>(*v);
	}

	/// Create data link initialized by zero vector of the given size.
	cow_data(size_t sz)
	{
#if COW_DEBUG
		countmalloc++;
#endif
		refcount=1;
		data=new std::vector<T>(sz);
	}

	/// Create data link initialized by vector of the given size and value.
	cow_data(size_t sz,const T& value)
	{
#if COW_DEBUG
		countmalloc++;
#endif
		refcount=1;
		data=new std::vector<T>(sz,value);
	}

	/// Remove data and link.
	~cow_data()
	{
#if COW_DEBUG
		countmalloc--;
		if(countmalloc==0)
			std::cout << "cow_vector debug: all allocated memory blocks released!!!! good :)" << endl;
		if(countmalloc < 0)
			std::cout << "COW_VECTOR DEBUG: DOUBLE FREE BUG!" << endl;
#endif
		delete data;
	}

};

#if COW_DEBUG
template<class T> int cow_data<T>::countmalloc=0;
#endif

template<class T> class cow_vector
{
	/// Helper vector to use create constant references to zero vector.
	static std::vector<T> zero_vector;
	
	/// Pointer to the vector data or 0 if this is a vector of zero length.
	cow_data<T>* p;

	/// Create empty vector if not yet exist.
	inline void check()
	{
		if(p==0)
		{
			p=new cow_data<T>();
#ifdef COW_DUMP_MALLOCS
			cout << "ITEM " << this << ": " << "Create " << p << " (" << p->refcount << ")" << endl;
#endif
		}
	}
	
	/// Free current reference if any.
	inline void free()
	{
		if(p==0)
			return;
		
#ifdef COW_DUMP_MALLOCS
		cout << "ITEM " << this << ": " << "Release " << p << " (" << p->refcount << ")" << endl;
#endif
		p->refcount--;
		if(p->refcount==0)
		{
#ifdef COW_DUMP_MALLOCS
			cout << "ITEM " << this << ": " << "Free " << p << endl;
#endif
			delete p;
		}
	}
	
	/// Create private copy if needed.
	inline void get_write_access()
	{
		if(p && p->refcount > 1)
		{
			cow_data<T>* newp=new cow_data<T>(p->data);
#ifdef COW_DUMP_MALLOCS
			cout << "ITEM " << this << ": " << "Clone " << p << " -> " << newp << endl;
#endif
			free();
			p=newp;
		}
	}
	
public:
	
	cow_vector()
	{
		p=0;
	}

	cow_vector(size_t sz)
	{
		p=new cow_data<T>(sz);
#ifdef COW_DUMP_MALLOCS
		cout << "ITEM " << this << ": " << "Create " << p << " (" << p->refcount << ")" << endl;
#endif
	}

	cow_vector(size_t sz,const T& value)
	{
		p=new cow_data<T>(sz,value);
#ifdef COW_DUMP_MALLOCS
		cout << "ITEM " << this << ": " << "Create " << p << " (" << p->refcount << ")" << endl;
#endif
	}
	
	~cow_vector()
	{
		free();
	}

	inline const cow_vector<T>& operator=(const cow_vector<T>& v)
	{
		free();
		p=v.p;
		if(p)
		{
			p->refcount++;
#ifdef COW_DUMP_MALLOCS
			cout << "ITEM " << this << ": " << "Copy " << p << " (" << p->refcount << ")" << endl;
#endif
		}
		
		return *this;
	}
	
	inline void push_back(const T& e)
	{
		check();
		get_write_access();
		p->data->push_back(e);
	}

	inline void insert(size_t pos,const T& e)
	{
		check();
		get_write_access();
		p->data->insert(p->data->begin()+pos,e);
	}

	inline void erase(size_t pos)
	{
		check();
		get_write_access();
		p->data->erase(p->data->begin()+pos);
	}

	inline const T& operator[](int i) const
	{
	  //	  cout << "CONST [" << i << "]" << endl;
		return (*p->data)[i];
	}
	
	inline T& operator[](int i)
	{
	  //	  cout << "REF [" << i << "]" << endl;
		check();
		get_write_access();
		return (*p->data)[i];
	}

	inline const std::vector<T>& const_ref() const
	{		
	    if(p==0)
		return zero_vector;
		
	    return *p->data;
	}

	inline std::vector<T>& ref()
	{
	    check();
	    get_write_access();
	    return *p->data;
	}

	inline typename std::vector<T>::const_iterator begin() const
	{
		if(p==0)
			return zero_vector.begin();
		
		return p->data->begin();
	}

	inline typename std::vector<T>::iterator begin()
	{
		if(p==0)
			return zero_vector.begin();

		get_write_access();
		
		return p->data->begin();
	}

	inline typename std::vector<T>::const_iterator end() const
	{
		if(p==0)
			return zero_vector.end();
		
		return p->data->end();
	}

	inline typename std::vector<T>::iterator end()
	{
		if(p==0)
			return zero_vector.end();
		
		get_write_access();
		
		return p->data->end();
	}
	
	inline size_t size() const
	{
		if(p)
			return p->data->size();
		else
			return 0;
	}

	inline T front() const
	{
		return (*p->data)[0];
	}
};

template<class T> std::ostream& operator<<(std::ostream& O,const cow_vector<T>& V)
{
	for(size_t i=0; i<V.size(); i++)
	{
		if(i)
			O << ' ';
		O << V[i];
	}

	return O;
}

template<class T> std::vector<T> cow_vector<T>::zero_vector;
