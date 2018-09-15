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



class oqmlIterator {
  oqmlDotContext *s_dctx;
  oqmlAtomList *s_and_ctx;

 protected:
  oqmlDotContext *dot_ctx;
  Database *db;
  oqmlAtom *start, *end;
  void *user_data;
  oqmlStatus *getValue(oqmlNode *, oqmlContext *, const Oid *, Data,
		       Data &, int &, Bool &);
  oqmlStatus *evalAnd(oqmlNode *, oqmlContext *, oqmlAtomList *,
		      oqmlBool (*)(Data, Bool, const oqmlAtom *,
				   const oqmlAtom *, int, void *),
		      oqmlAtomList *);
  oqmlStatus *evalAndRealize(oqmlNode *, oqmlContext *, oqmlAtom *,
			     oqmlBool (*)(Data, Bool, const oqmlAtom *,
					  const oqmlAtom *, int, void *),
			     oqmlAtomList *);
  oqmlStatus *begin(oqmlContext *);
  void commit(oqmlContext *);

 public:
  oqmlIterator(Database *, oqmlDotContext *, oqmlAtom *, oqmlAtom *,
	      void * = NULL);
  virtual oqmlStatus *eval(oqmlNode *, oqmlContext *, oqmlAtomList **) = 0;
};

class oqmlEqualIterator : public oqmlIterator {

 public:
  oqmlEqualIterator(Database *, oqmlDotContext *, oqmlAtom *, oqmlAtom *,
		   void * = NULL);
  oqmlStatus *eval(oqmlNode *, oqmlContext *, oqmlAtomList **);
};

class oqmlDiffIterator : public oqmlIterator {

 public:
  oqmlDiffIterator(Database *, oqmlDotContext *, oqmlAtom *, oqmlAtom *,
		  void * = NULL);
  oqmlStatus *eval(oqmlNode *, oqmlContext *, oqmlAtomList **);
};

class oqmlInfIterator : public oqmlIterator {

 public:
  oqmlInfIterator(Database *, oqmlDotContext *, oqmlAtom *, oqmlAtom *,
		 void * = NULL);
  oqmlStatus *eval(oqmlNode *, oqmlContext *, oqmlAtomList **);
};


class oqmlInfEqIterator : public oqmlIterator {

 public:
  oqmlInfEqIterator(Database *, oqmlDotContext *, oqmlAtom *, oqmlAtom *,
		 void * = NULL);
  oqmlStatus *eval(oqmlNode *, oqmlContext *, oqmlAtomList **);
};


class oqmlSupIterator : public oqmlIterator {

 public:
  oqmlSupIterator(Database *, oqmlDotContext *, oqmlAtom *, oqmlAtom *,
		 void * = NULL);
  oqmlStatus *eval(oqmlNode *, oqmlContext *, oqmlAtomList **);
};


class oqmlSupEqIterator : public oqmlIterator {

 public:
  oqmlSupEqIterator(Database *, oqmlDotContext *, oqmlAtom *, oqmlAtom *,
		   void * = NULL);
  oqmlStatus *eval(oqmlNode *, oqmlContext *, oqmlAtomList **);
};


class oqmlBetweenIterator : public oqmlIterator {

 public:
  oqmlBetweenIterator(Database *, oqmlDotContext *, oqmlAtom *, oqmlAtom *,
		      void * = NULL);
  oqmlStatus *eval(oqmlNode *, oqmlContext *, oqmlAtomList **);
};

class oqmlNotBetweenIterator : public oqmlIterator {

 public:
  oqmlNotBetweenIterator(Database *, oqmlDotContext *,
			 oqmlAtom *, oqmlAtom *, void * = NULL);
  oqmlStatus *eval(oqmlNode *, oqmlContext *, oqmlAtomList **);
};

class oqmlRegCmpIterator : public oqmlIterator {

 public:
  oqmlRegCmpIterator(Database *, oqmlDotContext *, oqmlAtom *, oqmlAtom *,
		    void * = NULL);
  oqmlStatus *eval(oqmlNode *, oqmlContext *, oqmlAtomList **);
};

class oqmlRegICmpIterator : public oqmlIterator {

 public:
  oqmlRegICmpIterator(Database *, oqmlDotContext *, oqmlAtom *, oqmlAtom *,
		     void * = NULL);
  oqmlStatus *eval(oqmlNode *, oqmlContext *, oqmlAtomList **);
};

class oqmlRegDiffIterator : public oqmlIterator {

 public:
  oqmlRegDiffIterator(Database *, oqmlDotContext *, oqmlAtom *, oqmlAtom *,
		     void * = NULL);
  oqmlStatus *eval(oqmlNode *, oqmlContext *, oqmlAtomList **);
};

class oqmlRegIDiffIterator : public oqmlIterator {

 public:
  oqmlRegIDiffIterator(Database *, oqmlDotContext *, oqmlAtom *, oqmlAtom *,
		      void * = NULL);
  oqmlStatus *eval(oqmlNode *, oqmlContext *, oqmlAtomList **);
};

class oqmlSubStrIterator : public oqmlIterator {

 public:
  oqmlSubStrIterator(Database *, oqmlDotContext *, oqmlAtom *, oqmlAtom *,
		     void * = NULL);
  oqmlStatus *eval(oqmlNode *, oqmlContext *, oqmlAtomList **);
};

