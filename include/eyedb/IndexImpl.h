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


#ifndef _EYEDB_INDEX_IMPL_H
#define _EYEDB_INDEX_IMPL_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  class BEMethod_C;
  class Dataspace;

  class IndexImpl : public gbxObject {
  
  public:
    enum Type {
      Hash = 1,
      BTree
    };

    /**
       Not yet documented
       @param type
       @param dataspace
       @param keycount_or_degree
       @param mth
       @param impl_hints
       @param impl_hints_cnt
    */
    IndexImpl(Type type, const Dataspace *dataspace = 0,
	      unsigned int keycount_or_degree = 0,
	      BEMethod_C *mth = 0,
	      const int impl_hints[] = 0,
	      unsigned int impl_hints_cnt = 0);

    /**
       Not yet documented
       @param type
       @param hints
       @param idximpl
       @param is_string
       @return
    */
    static Status make(Type type, const char *hints,
		       IndexImpl *&idximpl, Bool is_string = False) {
      return make(0, type, hints, idximpl, is_string);
    }

    // database is needed if hints contains a hash method
    /**
       Not yet documented
       @param db
       @param type
       @param hints
       @param idximpl
       @param is_string
       @return
    */
    static Status make(Database *db, Type type, const char *hints,
		       IndexImpl *&idximpl, Bool is_string = False);

    /**
       Not yet documented
       @return
    */
    Type getType() const {return type;}

    /**
       Not yet documented
       @return
    */
    const char *getStringType() const {return type == Hash ? "hash" : "btree";}

    /**
       Not yet documented
       @return
    */
    IndexImpl *clone() const;

    /**
       Not yet documented
       @return
    */
    const Dataspace *getDataspace() const {return dataspace;}

    /**
       Not yet documented
       @param idximpl
       @return
    */
    Bool compare(const IndexImpl *idximpl) const;

    /**
       Not yet documented
       @return
    */
    unsigned int getKeycount() const {return u.hash.keycount;}

    /**
       Not yet documented
       @return
    */
    BEMethod_C *getHashMethod() const {return u.hash.mth;}

    /**
       Not yet documented
       @return
    */
    std::string getHintsString() const;

    /**
       Not yet documented
       @param indent
       @return
    */
    std::string toString(const char *indent = "") const;

    /**
       Not yet documented
       @return
    */
    unsigned int getDegree() const {return u.degree;}

    /**
       Not yet documented
       @param _impl_hints_cnt
       @return
    */
    const int *getImplHints(unsigned int &_impl_hints_cnt) const {
      _impl_hints_cnt = impl_hints_cnt;
      return impl_hints;
    }

    ~IndexImpl();

    /**
       Not yet documented
       @param def_magorder
       @return
    */
    unsigned int getMagorder(unsigned int def_magorder) const;

    /**
       Not yet documented
       @param magorder
       @return
    */
    static unsigned int estimateHashKeycount(unsigned int magorder);

    /**
       Not yet documented
       @param magorder
       @return
    */
    static unsigned int estimateBTreeDegree(unsigned int magorder);

    /**
       Not yet documented
       @param hints
       @param cap
       @return
    */
    static const char *hashHintToStr(unsigned int hints, Bool cap = False);

    /**
       Not yet documented
       @param hints
       @return
    */
    static bool isHashHintImplemented(unsigned int hints);

  private:
    Type type;
    union {
      struct {
	unsigned int keycount;
	BEMethod_C *mth;
      } hash;
      unsigned int degree;
    } u;

    const Dataspace *dataspace;
    int *impl_hints;
    unsigned int impl_hints_cnt;

    static Status makeHash(Database *db, const char *hints,
			   IndexImpl *&idximpl, Bool is_string);
    static Status makeBTree(Database *db, const char *hints,
			    IndexImpl *&idximpl, Bool is_string);
    void garbage();

    IndexImpl(const IndexImpl &);
    IndexImpl& operator=(const IndexImpl &);

  public: // restricted access
    static const int UNKNOWN_IMPL_TYPE;
    static const int NOINDEX_IMPL_TYPE;
    static Status decode(Database *, Data, Offset &,
			 IndexImpl *&, int *rimpl_type = 0);
    static Status code(Data &, Offset &, Size &,
		       const IndexImpl *, int impl_type = UNKNOWN_IMPL_TYPE);
    void setHashMethod(BEMethod_C *);
    void setDataspace(const Dataspace *_d) {dataspace = _d;}
  };

  class HashIndexStats;
  class BTreeIndexStats;

  class IndexStats {

  public:
    IndexStats();
    virtual std::string toString(Bool dspImpl, Bool full, const char *indent = "") = 0;
    virtual HashIndexStats *asHashIndexStats() {return 0;}
    virtual BTreeIndexStats *asBTreeIndexStats() {return 0;}
    virtual ~IndexStats() = 0;

    IndexImpl *idximpl;
  };

  class HashIndexStats : public IndexStats {

  public:
    HashIndexStats();
    HashIndexStats *asHashIndexStats() {return this;}
    ~HashIndexStats();

    std::string toString(Bool dspImpl, Bool full, const char *indent = "");

    Status printEntries(const char *fmt, FILE *fd = stdout);

    unsigned int min_objects_per_entry;
    unsigned int max_objects_per_entry;
    unsigned int total_object_count;
    unsigned int total_hash_object_count;
    unsigned int total_hash_object_size;
    unsigned int total_hash_object_busy_size;
    unsigned int busy_key_count;
    unsigned int free_key_count;

    unsigned int key_count;
    struct Entry {
      unsigned int object_count;
      unsigned int hash_object_count;
      unsigned int hash_object_size;
      unsigned int hash_object_busy_size;
    } *entries;

    static const char fmt_help[];
  };

  class BTreeIndexStats : public IndexStats {

  public:
    BTreeIndexStats();
    ~BTreeIndexStats();
    std::string toString(Bool dspImpl, Bool full, const char *indent = "");
    virtual BTreeIndexStats *asBTreeIndexStats() {return this;}

    unsigned int degree;
    unsigned int dataSize;
    unsigned int keySize;
    unsigned int keyOffset;
    unsigned int keyType;

    unsigned int total_object_count;
    unsigned int total_btree_object_count;
    unsigned int btree_node_size;
    unsigned int total_btree_node_count;
    unsigned int btree_key_object_size;
    unsigned int btree_data_object_size;
    unsigned long long total_btree_object_size;
  };

  /**
     @}
  */

}

#endif
