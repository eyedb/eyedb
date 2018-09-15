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

public class TypeModifier {

    static final short _Indirect = 0x1;
    static final short _VarDim   = 0x2;

    public static final short Direct         = 0;
    public static final short Indirect       = _Indirect;
    public static final short VarDim         = _VarDim;
    public static final short IndirectVarDim = _Indirect | _VarDim;

    short mode;
    short ndims;
    int[] dims;
    int   pdims;
    int   maxdims;

    TypeModifier() {
    }

    TypeModifier(TypeModifier typmod) {
	mode    = typmod.mode;
	ndims   = typmod.ndims;
	dims    = typmod.dims;
	pdims   = typmod.pdims;
	maxdims = typmod.maxdims;
    }

    static TypeModifier make(boolean isref, int ndims, int[] dims)
    {
	TypeModifier typmod = new TypeModifier();

	typmod.mode = (isref ? _Indirect : 0);

	typmod.ndims   = (short)ndims;
	typmod.dims    = dims;
	typmod.pdims   = 1;
	typmod.maxdims = 1;

	if (ndims != 0) {

	    for (int n = 0; n < ndims; n++) {
		if (dims[n] < 0)   {
		    typmod.mode |= _VarDim;
		    typmod.maxdims *= -dims[n];
		}
		else  {
		    typmod.pdims *= dims[n];
		    typmod.maxdims *= dims[n];
		}
	    }
	}

	return typmod;
    }

    public short getMode() {return mode;}
    public short getNdims() {return ndims;}
    public int[] getDims() {return dims;}
    public int getPdims() {return pdims;}
    public int getMaxdims() {return maxdims;}

    void code(Coder coder) {
	coder.code(mode);
	coder.code(pdims);
	coder.code(maxdims);
	coder.code(ndims);

	for (int n = 0; n < ndims; n++)
	    coder.code(dims[n]);
    }

    public void trace() {
	System.out.println("mode " + mode);
	System.out.println("  ndims " +   ndims);
	System.out.println("  pdims " +   pdims);
	System.out.println("  maxdims " +   maxdims);
    }

    void decode(Coder coder) {
	mode    = coder.decodeShort();
	pdims   = coder.decodeInt();
	maxdims = coder.decodeInt();
	ndims   = coder.decodeShort();

	dims = new int[ndims];

	for (int n = 0; n < ndims; n++)
	    dims[n] = coder.decodeInt();
    }

    public boolean equals(java.lang.Object o) {
	if (!(o instanceof TypeModifier))
	    return false;

	TypeModifier typmod = (TypeModifier)o;
    
	if (ndims != typmod.ndims)
	    return false;
 
	if (pdims != typmod.pdims)
	    return false;
 
	if (maxdims != typmod.maxdims)
	    return false;

	if (mode != typmod.mode)
	    return false;

	for (int n = 0; n < ndims; n++)
	    if (dims[n] != typmod.dims[n])
		return false;
	
	return true;
    }

    private static boolean compare(int x1[], int x2[]) {
	if (x1 == null && x2 == null)
	    return true;

	if (x1 == null)
	    return x2.length == 0;

	if (x2 == null)
	    return x1.length == 0;

	if (x1.length != x2.length)
	    return false;

	for (int i = 0; i < x1.length; i++)
	    if (x1[i] != x2[i])
		return false;
	return true;
    }

    public boolean compare(TypeModifier typmod) {
	boolean r =
	    typmod.mode == mode &&
	    typmod.ndims == ndims &&
	    typmod.pdims == pdims &&
	    typmod.maxdims == maxdims &&
	    compare(typmod.dims, dims);

	return r;
    }
}
