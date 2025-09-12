/* -*- C++ -*- */

/****************************************************************************
** Copyright (c) 2001-2014
**
** This file is part of the QuickFIX FIX Engine
**
** This file may be distributed under the terms of the quickfixengine.org
** license as defined by quickfixengine.org and appearing in the file
** LICENSE included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.quickfixengine.org/LICENSE for licensing information.
**
** Contact ask@quickfixengine.org if any conditions of this licensing are
** not clear to you.
**
****************************************************************************/

#ifndef FIX_FIELDMAP
#define FIX_FIELDMAP

#ifdef _MSC_VER
#pragma warning( disable: 4786 )
#endif

#include "Field.h"
#include "MessageSorters.h"
#include "Exceptions.h"
#include "Utility.h"
#include <map>
#include <vector>
#include <sstream>
#include <algorithm>

namespace FIX
{
	/**
	 * Stores and organizes a collection of Fields.
	 *
	 * This is the basis for a message, header, and trailer.  This collection
	 * class uses a sorter to keep the fields in a particular order.
	 */
	class FieldMap
	{

		class sorter
		{
		public:
			explicit sorter(const message_order& order) : m_order(order) {}

			bool operator()(int tag, const FieldBase& right) const
			{
				return m_order(tag, right.getTag());
			}

			bool operator()(const FieldBase& left, int tag) const
			{
				return m_order(left.getTag(), tag);
			}

			bool operator()(const FieldBase& left, const FieldBase& right) const
			{
				return m_order(left.getTag(), right.getTag());
			}

		private:
			const message_order& m_order;
		};

		class finder
		{
		public:
			explicit finder(int tag) : m_tag(tag) {}

			bool operator()(const FieldBase& field) const
			{
				return m_tag == field.getTag();
			}

		private:
			int m_tag;
		};

		enum { DEFAULT_SIZE = 16 };

	protected:

		FieldMap(const message_order& order, int size);

	public:
		typedef std::vector < FieldBase, ALLOCATOR< FieldBase > > Fields;
		typedef std::map < int, std::vector < FieldMap* >, std::less<int>,
			ALLOCATOR<std::pair<const int, std::vector< FieldMap* > > > > Groups;

		class fields_iterator
		{
		public:
			using iterator_category = std::random_access_iterator_tag;
			using value_type = FieldBase;
			using difference_type = std::ptrdiff_t;
			using pointer = FieldBase*;
			using reference = FieldBase&;

			fields_iterator() = default;
			fields_iterator(FieldMap::Fields* global_fields, std::vector<int>* field_tags,
				FieldMap::Fields* fields, bool end_flag)
				: m_global_fields(global_fields)
				, m_field_tags(field_tags)
				, m_fields(fields)
			{
				if (!end_flag)
				{
					m_omd_it = m_field_tags->begin();
					m_fit = m_fields->begin();
				}
				else
				{
					m_omd_it = m_field_tags->end();
					m_fit = m_fields->end();
				}
			}

			reference operator*() const
			{
				if (m_global_fields != nullptr)
				{
					int tag = *m_omd_it;
					return (*m_global_fields)[tag];
				}
				else
				{
					return *m_fit;
				}
			}

			pointer operator->() const
			{
				return &(operator*());
			}

			fields_iterator& operator++()
			{
				if (m_global_fields != nullptr)
					++m_omd_it;
				else
					++m_fit;
				return *this;
			}

			fields_iterator operator++(int)
			{
				fields_iterator tmp = *this;
				if (m_global_fields != nullptr)
					++m_omd_it;
				else
					++m_fit;
				return tmp;
			}

			fields_iterator& operator--()
			{
				if (m_global_fields != nullptr)
					--m_omd_it;
				else
					--m_fit;
				return *this;
			}
			fields_iterator operator--(int)
			{
				fields_iterator tmp = *this;
				if (m_global_fields != nullptr)
					--m_omd_it;
				else
					--m_fit;
				return tmp; }

			bool operator==(const fields_iterator& other) const
			{
				if (m_global_fields != nullptr)
					return m_omd_it == other.m_omd_it;
				else
					return m_fit == other.m_fit;
			}
			bool operator!=(const fields_iterator& other) const
			{
				if (m_global_fields != nullptr)
					return m_omd_it != other.m_omd_it;
				else
					return m_fit != other.m_fit;
			}
			bool operator<(const fields_iterator& other) const
			{
				if (m_global_fields != nullptr)
					return m_omd_it < other.m_omd_it;
				else
					return m_fit < other.m_fit;
			}
			bool operator<=(const fields_iterator& other) const
			{
				if (m_global_fields != nullptr)
					return m_omd_it <= other.m_omd_it;
				else
					return m_fit <= other.m_fit;
			}
			bool operator>(const fields_iterator& other) const
			{
				if (m_global_fields != nullptr)
					return m_omd_it > other.m_omd_it;
				else
					return m_fit > other.m_fit;
			}
			bool operator>=(const fields_iterator& other) const
			{
				if (m_global_fields != nullptr)
					return m_omd_it >= other.m_omd_it;
				else
					return m_fit >= other.m_fit;
			}

		private:
			FieldMap::Fields* m_global_fields;
			std::vector<int>* m_field_tags;
			std::vector<int>::iterator m_omd_it;
			FieldMap::Fields* m_fields;
			FieldMap::Fields::iterator m_fit;
		};

		class const_fields_iterator
		{
		public:
			using iterator_category = std::random_access_iterator_tag;
			using value_type = const FieldBase;
			using difference_type = std::ptrdiff_t;
			using pointer = const FieldBase*;
			using reference = const FieldBase&;

			const_fields_iterator() = default;
			const_fields_iterator(const FieldMap::Fields* global_fields, const std::vector<int>* field_tags,
				const FieldMap::Fields* fields, bool end_flag)
				: m_global_fields(global_fields)
				, m_field_tags(field_tags)
				, m_fields(fields)
			{
				if (!end_flag)
				{
					m_omd_it = m_field_tags->begin();
					m_fit = m_fields->begin();
				}
				else
				{
					m_omd_it = m_field_tags->end();
					m_fit = m_fields->end();
				}
			}

			reference operator*() const
			{
				if (m_global_fields != nullptr)
				{
					int tag = *m_omd_it;
					return (*m_global_fields)[tag];
				}
				else
				{
					return *m_fit;
				}
			}

			pointer operator->() const
			{
				return &(operator*());
			}

			const_fields_iterator& operator++()
			{
				if (m_global_fields != nullptr)
					++m_omd_it;
				else
					++m_fit;
				return *this;
			}

			const_fields_iterator operator++(int)
			{
				const_fields_iterator tmp = *this;
				if (m_global_fields != nullptr)
					++m_omd_it;
				else
					++m_fit;
				return tmp;
			}

			const_fields_iterator& operator--()
			{
				if (m_global_fields != nullptr)
					--m_omd_it;
				else
					--m_fit;
				return *this;
			}
			const_fields_iterator operator--(int)
			{
				const_fields_iterator tmp = *this;
				if (m_global_fields != nullptr)
					--m_omd_it;
				else
					--m_fit;
				return tmp;
			}

			bool operator==(const const_fields_iterator& other) const
			{
				if (m_global_fields != nullptr)
					return m_omd_it == other.m_omd_it;
				else
					return m_fit == other.m_fit;
			}
			bool operator!=(const const_fields_iterator& other) const
			{
				if (m_global_fields != nullptr)
					return m_omd_it != other.m_omd_it;
				else
					return m_fit != other.m_fit;
			}
			bool operator<(const const_fields_iterator& other) const
			{
				if (m_global_fields != nullptr)
					return m_omd_it < other.m_omd_it;
				else
					return m_fit < other.m_fit;
			}
			bool operator<=(const const_fields_iterator& other) const
			{
				if (m_global_fields != nullptr)
					return m_omd_it <= other.m_omd_it;
				else
					return m_fit <= other.m_fit;
			}
			bool operator>(const const_fields_iterator& other) const
			{
				if (m_global_fields != nullptr)
					return m_omd_it > other.m_omd_it;
				else
					return m_fit > other.m_fit;
			}
			bool operator>=(const const_fields_iterator& other) const
			{
				if (m_global_fields != nullptr)
					return m_omd_it >= other.m_omd_it;
				else
					return m_fit >= other.m_fit;
			}

		private:
			const FieldMap::Fields* m_global_fields;
			const std::vector<int>* m_field_tags;
			std::vector<int>::const_iterator m_omd_it;
			const FieldMap::Fields* m_fields;
			FieldMap::Fields::const_iterator m_fit;
		};

		typedef fields_iterator iterator;
		typedef const_fields_iterator const_iterator;
		typedef Groups::iterator g_iterator;
		typedef Groups::const_iterator g_const_iterator;

		FieldMap(const message_order& order = message_order(message_order::normal));

		FieldMap(Fields* global_fields, const message_order& order = message_order(message_order::normal));

		FieldMap(const int order[]);

		FieldMap(const FieldMap& copy);

		virtual ~FieldMap();

		FieldMap& operator=(const FieldMap& rhs);

		/// Set a field without type checking
		void setField(const FieldBase& field, bool overwrite = true)
		{
			if (!overwrite)
			{
				addField(field);
			}
			else
			{
				FieldBase* field_ptr = findTagOmd(field.getTag());
				if (field_ptr == nullptr)
				{
					addField(field);
				}
				else
				{
					field_ptr->setString(field.getString());
				}
			}
		}

		/// Set a field without a field class
		void setField(int tag, const std::string& value)
		{
			FieldBase fieldBase(tag, value);
			setField(fieldBase);
		}

		/// Get a field if set
		bool getFieldIfSet(FieldBase& field) const
		{
			const FieldBase* field_ptr = findTagOmd(field.getTag());
			if (field_ptr == nullptr)
				return false;
			field = (*field_ptr);
			return true;
		}

		/// Get a field without type checking
		FieldBase& getField(FieldBase& field) const
		{
			field = getFieldRef(field.getTag());
			return field;
		}

		// OMD_THIRD_PARTY_CHANGE: mede this function virtual in order to override it in FIX::Message class and handle it correctly
		/// Get a field without a field class
		virtual const std::string& getField(int tag) const
		{
			return getFieldRef(tag).getString();
		}

		/// Get direct access to a field through a reference
		const FieldBase& getFieldRef(int tag) const
		{
			const FieldBase* field_ptr = findTagOmd(tag);
			if (field_ptr == nullptr)
				throw FieldNotFound(tag);
			return (*field_ptr);
		}

		/// Get direct access to a field through a pointer
		const FieldBase* const getFieldPtr(int tag) const
		{
			return &getFieldRef(tag);
		}

		/// Check to see if a field is set
		bool isSetField(const FieldBase& field) const
		{
			return isSetField(field.getTag());
		}

		// OMD_THIRD_PARTY_CHANGE: mede this function virtual in order to override it in FIX::Message class and handle it correctly
		/// Check to see if a field is set by referencing its number
		virtual bool isSetField(int tag) const
		{
			return findTagOmd(tag) != nullptr;
		}

		/// Remove a field. If field is not present, this is a no-op.
		void removeField(int tag);

		/// Add a group.
		void addGroup(int tag, const FieldMap& group, bool setCount = true);

		/// Acquire ownership of Group object
		void addGroupPtr(int tag, FieldMap* group, bool setCount = true);

		/// Replace a specific instance of a group.
		void replaceGroup(int num, int tag, const FieldMap& group);

		/// Get a specific instance of a group.
		FieldMap& getGroup(int num, int tag, FieldMap& group) const
		{
			return group = getGroupRef(num, tag);
		}

		/// Get direct access to a field through a reference
		FieldMap& getGroupRef(int num, int tag) const
		{
			Groups::const_iterator i = m_groups.find(tag);
			if (i == m_groups.end())
				throw FieldNotFound(tag);
			if (num <= 0)
				throw FieldNotFound(tag);
			if (i->second.size() < (unsigned)num)
				throw FieldNotFound(tag);
			return *(*(i->second.begin() + (num - 1)));
		}

		/// Get direct access to a field through a pointer
		FieldMap* getGroupPtr(int num, int tag) const
		{
			return &getGroupRef(num, tag);
		}

		/// Remove a specific instance of a group.
		void removeGroup(int num, int tag);
		/// Remove all instances of a group.
		void removeGroup(int tag);

		/// Check to see any instance of a group exists
		bool hasGroup(int tag) const;
		/// Check to see if a specific instance of a group exists
		bool hasGroup(int num, int tag) const;
		/// Count the number of instance of a group
		size_t groupCount(int tag) const;

		/// Clear all fields from the map
		void clear();
		/// Check if map contains any fields
		bool isEmpty();

		size_t totalFields() const;

		std::string& calculateString(std::string&) const;

		int calculateLength(int beginStringField = FIELD::BeginString, int bodyLengthField = FIELD::BodyLength,
			int checkSumField = FIELD::CheckSum) const;

		int calculateTotal(int checkSumField = FIELD::CheckSum) const;

		iterator begin()
		{
			return iterator(m_global_fields, &m_field_tags, &m_fields, false);
		}
		iterator end()
		{
			return iterator(m_global_fields, &m_field_tags, &m_fields, true);
		}
		const_iterator begin() const
		{
			return const_iterator(m_global_fields, &m_field_tags, &m_fields, false);
		}
		const_iterator end() const
		{
			return const_iterator(m_global_fields, &m_field_tags, &m_fields, true);
		}
		g_iterator g_begin()
		{
			return m_groups.begin();
		}
		g_iterator g_end()
		{
			return m_groups.end();
		}
		g_const_iterator g_begin() const
		{
			return m_groups.begin();
		}
		g_const_iterator g_end() const
		{
			return m_groups.end();
		}

	protected:

		friend class Message;

		void addField(const FieldBase& field)
		{
			int tag = field.getTag();
			if (m_global_fields != nullptr)
			{
				if (tag >= m_global_fields->size())
					throw RuntimeError("Internal error, vector index is out of range");
				(*m_global_fields)[tag] = field;
				m_field_tags.push_back(tag);
			}
			else
			{
				Fields::iterator iter = findPositionFor(tag);
				if (iter == m_fields.end())
					m_fields.push_back(field);
				else
					m_fields.insert(iter, field);
			}
		}

		// used to find data length fields during message decoding
		// message fields are not yet sorted so regular find*** functions might return wrong results
		const FieldBase& reverse_find(int tag) const
		{
			if (m_global_fields != nullptr)
			{
				const FieldBase* field = findTagOmd(tag);
				if (field == nullptr)
					throw FieldNotFound(tag);
				return *field;
			}
			else
			{
				Fields::const_reverse_iterator iter = std::find_if(m_fields.rbegin(), m_fields.rend(), finder(tag));
				if (iter == m_fields.rend())
					throw FieldNotFound(tag);
				return *iter;
			}
		}

		// append field to message without sorting
		// only applicable during message decoding
		void appendField(const FieldBase& field)
		{
			if (m_global_fields != nullptr)
			{
				addField(field);
			}
			else
			{
				m_fields.push_back(field);
			}
		}

		// sort fields after message decoding
		void sortFields()
		{
			std::sort(m_fields.begin(), m_fields.end(), sorter(m_order));
			std::sort(m_field_tags.begin(), m_field_tags.end());
		}

	private:
		Fields::iterator findTag(int tag)
		{
			return lookup(m_fields.begin(), m_fields.end(), tag);
		}

		const FieldBase* findTagOmd(int tag) const
		{
			if (m_global_fields != nullptr)
			{
				if ((*m_global_fields)[tag].m_initialized)
					return &(*m_global_fields)[tag];
				else
					return nullptr;
			}
			else
			{
				Fields::const_iterator it = lookup(m_fields.begin(), m_fields.end(), tag);
				if (it == m_fields.end())
					return nullptr;
				return &(*it);
			}
		}

		FieldBase* findTagOmd(int tag)
		{
			if (m_global_fields != nullptr)
			{
				if ((*m_global_fields)[tag].m_initialized)
					return &(*m_global_fields)[tag];
				else
					return nullptr;
			}
			else
			{
				Fields::iterator it = lookup(m_fields.begin(), m_fields.end(), tag);
				if (it == m_fields.end())
					return nullptr;
				return &(*it);
			}

		}

		template <typename Iterator>
		Iterator lookup(Iterator begin, Iterator end, int tag) const
		{
#if defined(__SUNPRO_CC)
			std::size_t numElements;
			std::distance(begin, end, numElements);
#else
			std::size_t numElements = std::distance(begin, end);
#endif
			if (numElements < 16)
				return std::find_if(begin, end, finder(tag));

			Iterator iter = std::lower_bound(begin, end, tag, sorter(m_order));
			if (iter != end && iter->getTag() == tag)
			{
				return iter;
			}

			return end;
		}

		Fields::iterator findPositionFor(int tag)
		{
			if (m_fields.empty())
				return m_fields.end();

			const FieldBase& last = m_fields.back();
			if (m_order(last.getTag(), tag) || last.getTag() == tag)
			{
				return m_fields.end();
			}

			return std::upper_bound(m_fields.begin(), m_fields.end(), tag, sorter(m_order));
		}

		Fields* m_global_fields;
		std::vector<int> m_field_tags;
		Fields m_fields;
		Groups m_groups;
		message_order m_order;
	};
	/*! @} */
}

#define FIELD_SET( MAP, FIELD )           \
bool isSet( const FIELD& field ) const    \
{ return (MAP).isSetField(field); }       \
void set( const FIELD& field )            \
{ (MAP).setField(field); }                \
FIELD& get( FIELD& field ) const          \
{ return (FIELD&)(MAP).getField(field); } \
bool getIfSet( FIELD& field ) const       \
{ return (MAP).getFieldIfSet(field); }

#define FIELD_GET_PTR( MAP, FLD ) \
(const FIX::FLD*)MAP.getFieldPtr( FIX::FIELD::FLD )
#define FIELD_GET_REF( MAP, FLD ) \
(const FIX::FLD&)MAP.getFieldRef( FIX::FIELD::FLD )
#define FIELD_THROW_IF_NOT_FOUND( MAP, FLD ) \
if( !(MAP).isSetField( FIX::FIELD::FLD) ) \
  throw FieldNotFound( FIX::FIELD::FLD )
#endif //FIX_FIELDMAP

