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

package org.eyedb;
import org.eyedb.syscls.BEMethod_C;

public class IndexImpl {

    static final int HASH = 1;
    static final int BTREE = 2;

    private int type;
    private Dataspace dataspace;
    private class HashIndex {
	int keycount;
	BEMethod_C mth;
    };

    HashIndex hash;

    private class BTreeIndex {
	int degree;
    };

    BTreeIndex btree;

    private int impl_hints[];

    public IndexImpl(int type, Dataspace dataspace,
			int keycount_or_degree,
			BEMethod_C mth, int impl_hints[]) {
	this.type = type;
	this.dataspace = dataspace;
	if (type == HASH) {
	    hash = new HashIndex();
	    hash.keycount = keycount_or_degree;
	    hash.mth = mth;
	}
	else {
	    btree = new BTreeIndex();
	    this.btree.degree = keycount_or_degree;
	}

	this.impl_hints = (impl_hints != null ? impl_hints : new int[0]);
    }

    public int getType() {return type;}

    public String getStringType() {
	return type == HASH ? "hash" : "btree";
    }

    public int getKeyCount() {return hash.keycount;}
    public int getDegree() {return btree.degree;}
    public BEMethod_C getHashMethod() {return hash.mth;}

    public int[] getImplHints() {return impl_hints;}

    public Dataspace getDataspace() {return dataspace;}

    static IndexImpl getDefaultIndexImpl() {
	return new IndexImpl(HASH, null, 2048, null, null);
    }

    static IndexImpl decode(Database db, Coder coder) throws Exception {
	int type = coder.decodeChar();
	short dspid = coder.decodeShort();
	int impl_info = coder.decodeInt();
	Oid mthoid = coder.decodeOid();
	Dataspace dataspace;
	Object mth;

	if (dspid != Root.DefaultDspid)
	    dataspace = db.getDataspace(dspid);
	else
	    dataspace = null;

	if (mthoid.isValid())
	    mth = db.loadObject(mthoid);
	else
	    mth = null;

	int impl_hints[] = new int[ObjectHeader.IDB_MAX_HINTS_CNT];

	for (int n = 0;
	     n < ObjectHeader.IDB_MAX_HINTS_CNT; n++)
	    impl_hints[n] = coder.decodeInt();

	return new IndexImpl(type, dataspace, impl_info,
			     (BEMethod_C)mth, impl_hints);
    }

    static void code(IndexImpl idximpl, Coder coder) {
	coder.code((char)idximpl.getType());
	short dspid = (short)(idximpl.getDataspace() != null ?
		       idximpl.getDataspace().getId() :
			      Root.DefaultDspid);
	coder.code(dspid);
	if (idximpl.getType() == HASH) {
	    coder.code(idximpl.getKeyCount());
	    BEMethod_C mth = idximpl.getHashMethod();
	    coder.code(mth != null ? mth.getOid() : Oid.nullOid);
	}
	else {
	    coder.code(idximpl.getDegree());
	    coder.code(Oid.nullOid);
	}

	int impl_hints[] = idximpl.getImplHints();
	//coder.code(impl_hints.length);
	for (int n = 0; n < impl_hints.length; n++)
	    coder.code(impl_hints[n]);

	for (int n = impl_hints.length;
	     n < ObjectHeader.IDB_MAX_HINTS_CNT; n++)
	    coder.code((int)0);
    }
}

