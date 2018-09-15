/* 
   EyeDB Object Database Management System
   Copyright (C) 1994-2008 SYSRA
   
   EyeDB is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   
   EyeDB is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   
   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA 
*/

/*
   Author: Eric Viara <viara@sysra.com>
*/


#ifndef _EYEDB_COLL_IMPL_H
#define _EYEDB_COLL_IMPL_H

#include "IndexImpl.h"
#include <assert.h>

namespace eyedb {
  
  class CollImpl : public gbxObject {

  public:
    enum Type {
      Unknown = 0,
      HashIndex = 1,
      BTreeIndex = 2,
      NoIndex = 3
    };

    CollImpl() : impl_type(Unknown), idximpl(0), dataspace(0),
		 impl_hints(0), impl_hints_cnt(0) { }

    CollImpl(CollImpl::Type impl_type, const Dataspace *dataspace = 0,
	     const int impl_hints[] = 0,
	     unsigned int impl_hints_cnt = 0,
	     unsigned int keycount_or_degree = 0,
	     BEMethod_C *mth = 0) :
      impl_type(impl_type), dataspace(dataspace) {
      if (impl_type == HashIndex ||
	  impl_type == BTreeIndex) {
	IndexImpl::Type idxtype = (impl_type == HashIndex ?
				   IndexImpl::Hash : IndexImpl::BTree);
	idximpl = new IndexImpl(idxtype, dataspace, keycount_or_degree, mth,
				impl_hints, impl_hints_cnt);
	this->impl_hints = 0;
	this->impl_hints_cnt = 0;
      }
      else {
	idximpl = 0;
	this->impl_hints_cnt = impl_hints_cnt;
	this->impl_hints = new int[impl_hints_cnt];
	memcpy(this->impl_hints, impl_hints, sizeof(int) * impl_hints_cnt);
      }
    }

    CollImpl(const IndexImpl *_idximpl) : idximpl(_idximpl), dataspace(0),
					  impl_hints(0), impl_hints_cnt(0) {
      if (!idximpl) {
	impl_type = NoIndex;
      }
      else if (idximpl->getType() == IndexImpl::Hash) {
	impl_type = HashIndex;
      }
      else if (idximpl->getType() == IndexImpl::BTree) {
	impl_type = BTreeIndex;
      }
      else {
	impl_type = Unknown;
      }
    }

    CollImpl(CollImpl::Type impl_type, const IndexImpl *_idximpl) :
      impl_type(impl_type), idximpl(_idximpl), dataspace(0),
      impl_hints(0), impl_hints_cnt(0) {
      // check
      if (!idximpl) {
	assert(impl_type == NoIndex ||
	       impl_type == Unknown);
      }
      else if (idximpl->getType() == IndexImpl::Hash) {
	assert(impl_type == HashIndex);
      }
      else if (idximpl->getType() == IndexImpl::BTree) {
	assert(impl_type == BTreeIndex);
      }
      else {
	assert(0);
      }
    }

    CollImpl::Type getType() const {return impl_type;}
    const IndexImpl *getIndexImpl() const {return idximpl;}
    
    void setType(CollImpl::Type impl_type) {this->impl_type = impl_type;}
    void setIndexImpl(const IndexImpl *idximpl) {this->idximpl = idximpl;}

    int *getImplHints(unsigned int &impl_hints_cnt) const {
      impl_hints_cnt = this->impl_hints_cnt; 
      return impl_hints;
    }
    
    bool compare(const CollImpl *collimpl) const {
      if (impl_type != collimpl->impl_type) {
	return false;
      }

      if (impl_type == NoIndex) {
	// must compare:
	// dataspace
	// hints
      }
      else {
	if ((idximpl == NULL && collimpl->idximpl != NULL) ||
	    (idximpl != NULL && collimpl->idximpl == NULL)) {
	  return false;
	}
	
	if (idximpl != NULL && collimpl->idximpl != NULL) {
	  return (bool)idximpl->compare(collimpl->idximpl);
	}
      }

      return true;
    }

    CollImpl *clone() const;

    ~CollImpl() {
      delete [] impl_hints;
    }

  private:
    Type impl_type;
    const IndexImpl *idximpl;
    const Dataspace *dataspace;
    int *impl_hints;
    unsigned int impl_hints_cnt;
  };
}

#endif
