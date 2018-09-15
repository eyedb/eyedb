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


#ifndef _EYEDB_ENUM_CLASS_H
#define _EYEDB_ENUM_CLASS_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class EnumItem {

    // ----------------------------------------------------------------------
    // EnumItem Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param nm
       @param val
       @param num
    */
    EnumItem(const char *nm, unsigned int val, int num = -1);

    /**
       Not yet documented
       @param nm
       @param aliasname
       @param val
       @param num
    */
    EnumItem(const char *nm, const char *aliasname, unsigned int val = 0, int num = -1);

    /**
       Not yet documented
       @return
    */
    unsigned int getValue() const {return value;}

    /**
       Not yet documented
       @return
    */
    const char *getName() const   {return name;}

    /**
       Not yet documented
       @return
    */
    const char *getAliasName() const   {return aliasname ? aliasname : name;}

    /**
       Not yet documented
       @param item
       @return
    */
    Bool compare(const EnumItem *item) const;

    /**
       Not yet documented
       @return
    */
    EnumItem *clone() const;

    ~EnumItem();

    // ----------------------------------------------------------------------
    // EnumItem Private Part
    // ----------------------------------------------------------------------
  private:
    friend class EnumClass;
    friend class Enum;
    char *name;
    char *aliasname;
    unsigned int value;
    int num;
    EnumItem(const EnumItem *, int);
  };

  class EnumClass : public Class {

    // ----------------------------------------------------------------------
    // EnumClass Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented
       @param s
    */
    EnumClass(const char *s);

    /**
       Not yet documented
       @param db
       @param s
    */
    EnumClass(Database *db, const char *s);

    /**
       Not yet documented
       @param cl
    */
    EnumClass(const EnumClass &cl);

    /**
       Not yet documented
       @param cl
       @return
    */
    EnumClass& operator=(const EnumClass &cl);

    /**
       Not yet documented
       @return
    */
    virtual Object *clone() const {return new EnumClass(*this);}

    /**
       Not yet documented
       @return
    */
    int getEnumItemsCount() const;

    /**
       Not yet documented
       @param n
       @return
    */
    const EnumItem *getEnumItem(int n) const;

    /**
       Not yet documented
       @param nm
       @return
    */
    const EnumItem *getEnumItemFromName(const char *nm) const;

    /**
       Not yet documented
       @param val
       @return
    */
    const EnumItem *getEnumItemFromVal(unsigned int val) const;

    /**
       Not yet documented
       @param nitems
       @param cnt
       @return
    */
    Status setEnumItems(EnumItem **nitems, int cnt);

    /**
       Not yet documented
       @param cnt
       @return
    */
    const EnumItem **getEnumItems(int &cnt) const;

    /**
       Not yet documented
       @param db
       @return
    */
    Object *newObj(Database *db = NULL) const;

    /**
       Not yet documented
       @param data
       @param copy
       @return
    */
    Object *newObj(Data data, Bool copy = True) const;

    /**
       Not yet documented
       @param s
       @return
    */
    Status setName(const char *s);

    /**
       Not yet documented
    */
    void touch();

    /**
       Not yet documented
       @return
    */
    virtual EnumClass *asEnumClass() {return this;}

    /**
       Not yet documented
       @return
    */
    virtual const EnumClass *asEnumClass() const {return this;}

    void _setCSDRSize(Size, Size);

    /**
       Not yet documented
       @param cl
       @return
    */
    Bool compare_perform(const Class *cl,
			 Bool compClassOwner,
			 Bool compNum,
			 Bool compName,
			 Bool inDepth) const;
    //Bool compare_perform(const Class *cl) const;

    /**
       Not yet documented
       @param fd
       @param flags
       @param recmode
       @return
    */
    Status trace(FILE *fd = stdout, unsigned int flags = 0, const RecMode *recmode = RecMode::FullRecurs) const;

    Status setValue(Data);
    Status getValue(Data*) const;

    /**
       Not yet documented
       @return
    */
    Status create();

    Status update();

    Status remove(const RecMode* = RecMode::NoRecurs);

    Status generateCode_C(Schema *, const char *prefix,
			  const GenCodeHints &,
			  const char *stubs,
			  FILE *, FILE *, FILE *, FILE *, FILE *, FILE *);
    Status generateCode_Java(Schema *, const char *prefix, 
			     const GenCodeHints &, FILE *);
    Status traceData(FILE *, int, Data, Data, TypeModifier * = NULL) const;
    Status setRawData(Data, Data, int, Bool &, Bool);
    Status getRawData(Data, Data, int) const;

    virtual ~EnumClass();

    // ----------------------------------------------------------------------
    // EnumClass Private Part
    // ----------------------------------------------------------------------
  private:
    int		  items_cnt;
    EnumItem **  items;

    void free_items();

    Status generateClassDesc_C(GenContext *);

    Status generateClassDesc_Java(GenContext *);

    friend Status
    enumClassMake(Database *, const Oid *, Object **,
		  const RecMode *, const ObjectHeader *, Data,
		  LockMode, const Class *);

    Status trace_realize(FILE*, int, unsigned int, const RecMode *) const;

    int genODL(FILE *fd, Schema *) const;

    virtual void garbage();
    void copy_realize(const EnumClass &cl);

    // ----------------------------------------------------------------------
    // EnumClass Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    EnumClass(const Oid&, const char *);
    virtual Status loadComplete(const Class *);
  };

  class EnumClassPtr : public ClassPtr {

  public:
    EnumClassPtr(EnumClass *o = 0) : ClassPtr(o) { }

    EnumClass *getEnumClass() {return dynamic_cast<EnumClass *>(o);}
    const EnumClass *getEnumClass() const {return dynamic_cast<EnumClass *>(o);}

    EnumClass *operator->() {return dynamic_cast<EnumClass *>(o);}
    const EnumClass *operator->() const {return dynamic_cast<EnumClass *>(o);}
  };

  typedef std::vector<EnumClassPtr> EnumClassPtrVector;

  /**
     @}
  */

}

#endif
