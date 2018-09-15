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


#ifndef _EYEDB_AGREGAT_H
#define _EYEDB_AGREGAT_H

namespace eyedb {

  /**
     @addtogroup eyedb
     @{
  */

  /**
     Not yet documented.
  */
  class Agregat : public Instance {

    // ----------------------------------------------------------------------
    // Agregat Interface
    // ----------------------------------------------------------------------
  public:
    /**
       Not yet documented.
       @param db
       @param dataspace
    */
    Agregat(Database *db = 0, const Dataspace *dataspace = 0);

    /**
       Not yet documented.
       @param o
    */
    Agregat(const Agregat &o);

    /**
       Not yet documented.
       @param o
       @param share
    */
    Agregat(const Agregat *o, Bool share = False);

    /**
       Not yet documented.
       @param o
    */
    Agregat& operator=(const Agregat &o);

    /**
       Not yet documented.
       Bizarre, n'est pas dans le .cc
       @param data
       @return
    */
    Status setValue(Data data);

    /**
       Not yet documented.
       Idem, bizarre, n'est pas dans le .cc
       @param data
       @return
    */
    Status getValue(Data* data) const;

    /**
       Not yet documented.
       @param agreg
       @param size
       @return
    */
    Status setItemSize(const Attribute* agreg, Size size);

    /**
       Not yet documented.
       @param agreg
       @param psize
       @return
    */
    Status getItemSize(const Attribute* agreg, Size* psize) const;

    /**
       Not yet documented.
       @param agreg
       @param data
       @param nb
       @param from
       @return
    */
    Status setItemValue(const Attribute* agreg, Data data, int nb=1, int from=0);

    /**
       Not yet documented.
       @param agreg
       @param poid
       @param nb
       @param from
       @return
    */
    Status setItemOid(const Attribute* agreg, const Oid * poid, int nb=1, int from=0, Bool check_class = True);

    /**
       Not yet documented.
       @param agreg
       @param poid
       @param nb
       @param from
       @return
    */
    Status getItemOid(const Attribute *agreg, Oid *poid, int nb = 1, int from = 0) const;

    /**
       Not yet documented.
       @param recmode
       @return
    */
    virtual Status realize(const RecMode* recmode = RecMode::NoRecurs);

    /**
       Not yet documented.
       @param recmode
       @return
    */
    virtual Status remove(const RecMode* recmode = RecMode::NoRecurs);

    /**
       Not yet documented.
       @param fd
       @param flags
       @param recmode
       @return
    */
    virtual Status trace(FILE* fd = stdout, unsigned int flags = 0,
			 const RecMode * recmode = RecMode::FullRecurs) const;

    /**
       Not yet documented.
       @return
    */
    virtual Agregat *asAgregat() {return this;}

    /**
       Not yet documented.
       @return
    */
    virtual const Agregat *asAgregat() const {return this;}

    /**
    */
    virtual ~Agregat();

    // ----------------------------------------------------------------------
    // Agregat Protected Part
    // ----------------------------------------------------------------------
  protected:
    Status checkAgreg(const Attribute*) const;
    Status create();
    Status update();
    virtual void garbage();
#ifdef GBX_NEW_CYCLE
    virtual void decrRefCountPropag();
#endif
    void initialize(Database *);

    // ----------------------------------------------------------------------
    // Agregat Private Part
    // ----------------------------------------------------------------------
  private:
    void copy(const Agregat *, Bool);

    Status trace_realize(FILE*, int, unsigned int, const RecMode *) const;
    Status create_realize(Bool);
    Status update_realize(Bool);

    // ----------------------------------------------------------------------
    // Agregat Restricted Access (conceptually private)
    // ----------------------------------------------------------------------
  public:
    virtual void manageCycle(gbxCycleContext &);

    Status realizePerform(const Oid& cloid,
			  const Oid& objoid,
			  AttrIdxContext &idx_ctx,
			  const RecMode *);
    Status loadPerform(const Oid& cloid,
		       LockMode,
		       AttrIdxContext &idx_ctx,
		       const RecMode* = RecMode::NoRecurs);
    Status removePerform(const Oid& cloid,
			 const Oid& objoid,
			 AttrIdxContext &idx_ctx,
			 const RecMode *);
  };

  class AgregatPtr : public InstancePtr {

  public:
    AgregatPtr(Agregat *o = 0) : InstancePtr(o) { }

    Agregat *getAgregat() {return dynamic_cast<Agregat *>(o);}
    const Agregat *getAgregat() const {return dynamic_cast<Agregat *>(o);}

    Agregat *operator->() {return dynamic_cast<Agregat *>(o);}
    const Agregat *operator->() const {return dynamic_cast<Agregat *>(o);}
  };

  typedef std::vector<AgregatPtr> AgregatPtrVector;

  /**
     @}
  */

}

#endif
