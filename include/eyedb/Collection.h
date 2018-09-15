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


#ifndef _EYEDB_COLLECTION_H
#define _EYEDB_COLLECTION_H

#define USE_VALUE_CACHE

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  class ValueCache;
  class CollectionPeer;
  class CardinalityDescription;
  class CollImpl;

  /**
     Not yet documented.
  */
  class Collection : public Instance {

    // ----------------------------------------------------------------------
    // Collection Interface
    // ----------------------------------------------------------------------
  public:

    typedef eyedblib::uint32 ItemId;

    /**
       Not yet documented
       @return
    */
    int getCount() const;

    /**
       Not yet documented
       @return
    */
    Bool isEmpty() const;

    /**
       Not yet documented
       @return
    */
    const char *getName() const {return name;}

    /**
       Not yet documented
       @param s
    */
    void setName(const char *s);

    /**
       Not yet documented
       @return
    */
    Status getStatus() const;

    static const Size defaultSize;

    /**
       Not yet documented
       @param v
       @param noDup
       @return
    */
    virtual Status insert(const Value &v, Bool noDup = False);

    /**
       Not yet documented
       @param item_value
       @param checkFirst
       @return
    */
    virtual Status suppress(const Value &item_value, Bool checkFirst = False);

    /**
       Not yet documented
       @return
    */
    Status empty();


    /**
       Not yet documented
       @param value
       @param found
       @param where
       @return
    */
    virtual Status isIn(const Value &value, Bool &found,
			Collection::ItemId *where = 0) const;

    /**
       Not yet documented
       @param oid_array
       @return
    */
    Status getElements(OidArray &oid_array) const;

    /**
       Not yet documented
       @param obj_vect
       @param recmode
       @return
    */
    Status getElements(ObjectPtrVector &obj_vect,
		       const RecMode *recmode = RecMode::NoRecurs) const;

    /**
       Not yet documented
       @param obj_array
       @param recmode
       @return
    */
    Status getElements(ObjectArray &obj_array,
		       const RecMode *recmode = RecMode::NoRecurs) const;

    /**
       Not yet documented
       @param value_array
       @param index
       @return
    */
    Status getElements(ValueArray &value_array, Bool index = False) const;


    /**
       Not yet documented
       @param recmode
       @return
    */
    Status realize(const RecMode *recmod = RecMode::NoRecurs);

    /**
       Not yet documented
       @param recmode
       @return
    */
    Status remove(const RecMode *recmode = RecMode::NoRecurs);

    /**
       Not yet documented
       @param fd
       @param flags
       @param recmode
       @return
    */
    Status trace(FILE *fd = stdout, unsigned int flags = 0,
		 const RecMode *recmode = RecMode::FullRecurs) const;

    /**
       Not yet documented
       @param collimpl
    */
    void setImplementation(const CollImpl *collimpl);

    /**
       Not yet documented
       @param collimpl
       @param remote
       @return
    */
    Status getImplementation(CollImpl *&collimpl, Bool remote = False) const;

    /**
       Not yet documented
       @param xstats
       @param dspImpl
       @param full
       @param indent
       @return
    */
    Status getImplStats(std::string &xstats, Bool dspImpl = True,
			Bool full = False, const char *indent = "");

    /**
       Not yet documented
       @param stats
       @return
    */
    Status getImplStats(IndexStats *&stats);

    /**
       Not yet documented
       @param collimpl
       @param xstats
       @param dspImpl
       @param full
       @param indent
       @return
    */
    Status simulate(const CollImpl &collimpl, std::string &xstats,
		    Bool dspImpl = True, Bool full = False,
		    const char *indent = "");

    /**
       Not yet documented
       @param collimpl
       @param stats
       @return
    */
    Status simulate(const CollImpl &collimpl, IndexStats *&stats);

    /**
       Not yet documented
       @param mdb
       @return
    */
    virtual Status setDatabase(Database *mdb);

    /**
       Not yet documented
    */
    virtual void garbage();

    /**
       Not yet documented
       @param card
    */
    void setCardinalityConstraint(Object *card);

    /**
       Not yet documented
       @return
    */
    CardinalityDescription *getCardinalityConstraint() const;

    /**
       Not yet documented
       @return
    */
    Status checkCardinality() const;

    Status realizeCardinality();

    /**
       Not yet documented
       @return
    */
    int getBottom() const;

    /**
       Not yet documented
       @return
    */
    int getTop() const;

    /**
       Not yet documented
       @return
    */
    virtual Collection *asCollection() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const Collection *asCollection() const {return this;}

    /**
       Not yet documented
       @param dataspace
       @return
    */
    Status getDefaultDataspace(const Dataspace *&dataspace) const;

    /**
       Not yet documented
       @param dataspace
       @return
    */
    Status setDefaultDataspace(const Dataspace *dataspace);

    /**
       Not yet documented
       @param dataspace
       @return
    */
    Status moveElements(const Dataspace *dataspace);

    Status getElementLocations(ObjectLocationArray &);

    Bool isLiteral() const {return is_literal;}
    Bool isPureLiteral() const {return is_pure_literal;}
    Bool isLiteralObject() const;

    Status setLiteralObject(bool force);

    virtual ~Collection();

    enum State {
      coherent = 1,
      added,
      removed,
      modified
    };

    // ----------------------------------------------------------------------
    // Collection Protected Part
    // ----------------------------------------------------------------------
  protected:
    eyedblib::int32 type;
    char *name;
    Bool ordered;
    Bool allow_dup;
    Bool string_coll;
    int bottom, top;
    Bool implModified;
    Bool nameModified; 

    Class *coll_class;
    Bool isref;
    eyedblib::int16 dim;
    eyedblib::int16 item_size;
    Oid cl_oid;

    Bool locked;
    CollImpl *collimpl;
    Oid inv_oid;
    eyedblib::int16 inv_item;

    Bool is_literal;
    Bool is_pure_literal;
    Oid literal_oid;
    Size idx_data_size;
    Data idx_data;

    CardinalityDescription *card;
    int card_bottom, card_bottom_excl, card_top, card_top_excl;
    Oid card_oid;

    Oid idx1_oid, idx2_oid;
    eyedbsm::Idx *idx1, *idx2; // back end only
    int p_items_cnt;
    int v_items_cnt;
    ValueCache *cache;
    Bool is_complete;
    void create_cache();

    struct ReadCache {
      ObjectArray *obj_arr;
      OidArray *oid_arr;
      ValueArray *val_arr;
    } read_cache;

    Status status;
    Data make_data(Data, Size, Bool swap = False) const;
    void decode(Data) const;

    Collection(const char *, Class * = NULL, Bool = True,
	       const CollImpl * = 0);
    Collection(const char *, Class *, int, const CollImpl * = 0);

    Status check(const Oid&, const Class *, Error) const;
    Status check(const Oid&, Error) const;
    Status check(const Object*, Error) const;
    Status check(Data, Size, Error) const;
    Status check(const Value &, Error) const;

    Status trace_realize(FILE*, int, unsigned int, const RecMode *) const;
    Status trace_contents_realize(FILE*, int, unsigned int, const RecMode *) const;
    Status getIdxOid(Oid &idx1oid, Oid &idx2oid) const;
    void completeImplStats(IndexStats *stats) const;

    void isStringColl();

    Collection(const char *, Class *,
	       const Oid&, const Oid&, int, int,
	       int, const CollImpl *,
	       Object *, Bool, Bool, Data, Size);
    void make(const char *, Class *, Bool, const CollImpl *);
    void make(const char *, Class *, int, const CollImpl *);
    Status getOidElementsRealize();
    Status getObjElementsRealize(const RecMode * = RecMode::NoRecurs);
    Status getValElementsRealize(Bool index = False);

    // ----------------------------------------------------------------------
    // Collection Private Part
    // ----------------------------------------------------------------------
  private:
    Collection::State read_cache_state_oid, read_cache_state_object,
      read_cache_state_value;
    Bool read_cache_state_index;
    Bool inverse_valid;
    virtual Status cache_compile(Offset &, Size&, unsigned char **, const RecMode *);
    void _init(const CollImpl *);
    Status init_idr();
    virtual const char *getClassName() const = 0;
    friend class CollectionPeer;

    Status codeCollImpl(Data &, Offset &, Size &);
    Status setValue(Data);
    Status getValue(Data*) const;
    void cardCode(Data &, Offset &offset, Size &alloc_size);
    Status failedCardinality() const;
    Status create();
    Status update();
    Status updateLiteral();
    Status loadLiteral();

    enum {
      CollObject = 0,
      CollPureLiteral = 1,
      CollLiteral = 2
    };
    char codeLiteral() const;
    Status literalMake(Collection *o);
    std::string getStringType() const;
    Offset inv_oid_offset;

    // ----------------------------------------------------------------------
    // Collection Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    Collection(const Collection &);
    Collection& operator=(const Collection &);
    static Object *cardDecode(Database *, Data, Offset &);
    Status realizeInverse(const Oid&, int);
    void setInverse(const Oid&, int);
    void invalidateInverse() {inverse_valid = False;}
    void validateInverse() {inverse_valid = True;}
    void unvalidReadCache();
    void emptyReadCache();
    void setLiteral(Bool _is_literal);
    void setPureLiteral(Bool _is_pure_literal);
    Oid getLiteralOid() const {return literal_oid;}
    void setLiteralOid(Oid _literal_oid) {literal_oid = _literal_oid;}
    Oid& getOidC() {
      return is_literal ? literal_oid : oid;
    }
    const Oid& getOidC() const {
      return is_literal ? literal_oid : oid;
    }

    static void decodeLiteral(char c, Bool &is_literal, Bool &is_pure_literal);
    Bool isPartiallyStored() const;
    virtual Status setMasterObject(Object *_master_object);
    virtual Status releaseMasterObject();

    void makeValue(Value &);

    Status create_realize(const RecMode *);
    Status update_realize(const RecMode *);
    Status realizePerform(const Oid& cloid,
			  const Oid& objoid,
			  AttrIdxContext &idx_ctx,
			  const RecMode *);
    Status loadPerform(const Oid&,
		       LockMode lockmode,
		       AttrIdxContext &idx_ctx,
		       const RecMode* = RecMode::NoRecurs);
    Status removePerform(const Oid& cloid,
			 const Oid& objoid,
			 AttrIdxContext &idx_ctx,
			 const RecMode *);
    Status loadDeferred(LockMode lockmode = DefaultLock,
			const RecMode * = RecMode::NoRecurs);
    Status postRealizePerform(const Oid& cloid,
			      const Oid& objoid,
			      AttrIdxContext &idx_ctx,
			      Bool&,
			      const RecMode *rcm);
    virtual Status insert_p(const Oid &item_oid, Bool noDup = False) = 0;
    virtual Status insert_p(const Object *o, Bool noDup = False) = 0;
    virtual Status insert_p(Data val, Bool noDup = False,
			    Size size = defaultSize) = 0;
    virtual Status suppress_p(const Oid &item_oid, Bool checkFirst = False);
    virtual Status suppress_p(const Object *item_o, Bool checkFirst = False);
    virtual Status suppress_p(Data data, Bool checkFirst = False,
			      Size size = defaultSize);
    virtual Status isIn_p(const Oid &item_oid, Bool &found,
			  Collection::ItemId *where = 0) const;
    virtual Status isIn_p(const Object *item_o, Bool &found,
			  Collection::ItemId *where = 0) const;
    Status isIn_p(Data data, Bool &found, Size size = defaultSize,
		  Collection::ItemId *where = 0) const;

  };

  class CollectionPtr : public InstancePtr {

  public:
    CollectionPtr(Collection *o = 0) : InstancePtr(o) { }

    Collection *getCollection() {return dynamic_cast<Collection *>(o);}
    const Collection *getCollection() const {return dynamic_cast<Collection *>(o);}

    Collection *operator->() {return dynamic_cast<Collection *>(o);}
    const Collection *operator->() const {return dynamic_cast<Collection *>(o);}
  };

  typedef std::vector<CollectionPtr> CollectionPtrVector;

  /**
     @}
  */

}

#endif
