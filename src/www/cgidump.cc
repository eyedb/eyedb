/* 
   EyeDB Object Database Management System
   Copyright (C) 1994-2018 SYSRA
   
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


#include "eyedbcgiP.h"
#include "eyedb/Iterator.h" // eyedb private include

#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))

#define NCOLS     4
#define MAXSIZE  80

#define DG_SCENTER "<center>"
#define DG_ECENTER "</center>"

//#define DG_SCENTER ""
//#define DG_ECENTER ""

//#define USEFORM

const char idbW_unknownTag[] = "??";
const char idbW_skipTag[] = "  ";
int idbW_start_tmp, idbW_ostart_tmp;

struct idbWDumpCollContext {
  const Class *cls;
  Oid colloid;
  int collcount;
  int dumpedcount;
  int maxcount, maxcount_orig;
  int totalcount;
  int totaldumpedcount;
  std::string name;
  const char **tags;
  int *lens;
  Object **objs;
  int maxlen;
  int start, start_orig;
  Database *db;
  int indent;
  int lastcount;
  Bool ok;
  int collelemcount;

  idbWDumpCollContext(const Class *_cls, Collection *&coll, int _start,
		      const char *_name, Bool standalone = True) {
    cls = _cls;
    colloid = coll->getOidC();
    collcount = coll->getCount();
    db = coll->getDatabase();
    collelemcount = standalone ? idbW_db_config->display_config.collelemcount : 10;
    maxcount_orig = maxcount = collelemcount;
    totalcount = (cls ? idbW_get_instance_number(cls, True) :
		  coll->getCount());
    tags = new const char *[maxcount];
    lens = new int[maxcount];
    objs = new Object*[maxcount];
    dumpedcount = 0;
    maxlen = 0;
    start_orig = start = _start;
    totaldumpedcount = idbW_ctx->totaldumpedcount_str ?
      atoi(idbW_ctx->totaldumpedcount_str) : 0;
    lastcount = 0;

    if (cls)
      name = std::string(cls->getName()) + " extent";
    else
      name = _name; // coll->getName() ?

    ok = False;

    if (cls)
      {
	sync_extents();
	coll->release();
	Collection *_coll;
	Status s = db->loadObject(colloid, (Object *&)_coll);
	if (s)
	  {
	    idbW_STPR(s);
	    return;
	  }

	coll = _coll;
      }
  }

  void sync_extents() {
    Class **classes;
    unsigned int cnt;
    cls->getSubClasses(classes, cnt);
    for (int i = 0; i < cnt; i++)
      {
	Collection *extent;
	classes[i]->getExtent(extent, True); // to sync extent with the DB
      }
  }

  unsigned int getPrevContext(Oid &prevoid) {
    if (!cls)
      {
	prevoid = colloid;
	int s = start_orig-collelemcount;
	if (s < 0)
	  s = 0;
	return (unsigned int)s;
      }

    int cnt = collelemcount;
    if (start_orig >= cnt)
      {
	prevoid = colloid;
	return start_orig - cnt;
      }
    
    int elemcnt = start_orig - cnt;
    return get_prev_coll(elemcnt, prevoid);
  }

  ~idbWDumpCollContext() {
    int i;
    for (i = 0; i < dumpedcount; i++)
      free((void *)tags[i]);
    delete [] tags;
    delete [] lens;
    for (i = 0; i < dumpedcount; i++)
      if (objs[i]) objs[i]->release();
    delete [] objs;
  }

  void trace(const char *s) {
    printf("DumpContext %s {\n", s);
    printf("\tclass = %s;\n", cls ? cls->getName() : "<none>");
    printf("\tcolloid = %s\n", colloid.toString());
    printf("\tcollcount = %d\n", collcount);
    printf("\tdumpedcount = %d\n", dumpedcount);
    printf("\tmaxcount = %d\n", maxcount);
    printf("\tmaxcount_orig = %d\n", maxcount_orig);
    printf("\ttotalcount = %d\n", totalcount);
    printf("\ttotaldumpedcount = %d\n", totaldumpedcount);
    printf("\tlastcount = %d\n", lastcount);
    printf("\tmaxlen = %d\n", maxlen);
    printf("\tstart = %d\n", start);
    printf("\tstart_orig = %d\n", start_orig);
    printf("\tname = \"%s\";\n", name.c_str());
    printf("\tisok = %s;\n", isOk() ? "true" : "false");
    printf("}\n");
  }

  void add(int _count) {
    dumpedcount += _count;
    maxcount -= _count;
    lastcount = _count;
  }

  Bool isOk() const {
    return IDBBOOL(ok || !cls || (maxcount <= 0));
  }

  unsigned int get_prev_coll(int elemcnt, Oid &prevoid) {

    Class **classes;
    unsigned int cnt;
    cls->getSubClasses(classes, cnt);

    for (int n = cnt-1; n >= 0; n--)
      {
	Collection *extent;
	classes[n]->getExtent(extent);
	if (colloid == extent->getOidC())
	  {
	    if (n >= 1)
	      {
		classes[n-1]->getExtent(extent);
		elemcnt += extent->getCount();
		if (elemcnt >= 0)
		  {
		    prevoid = extent->getOidC();
		    return elemcnt;
		  }
	      }
	  }
      }

    abort();
    return 0;
  }

  Bool get_next_coll_realize(const Class *xcls, Collection *&_coll) {
    Class **classes;
    unsigned int cnt;
    xcls->getSubClasses(classes, cnt);

    Oid coll_oid = _coll->getOidC();
    for (int n = 0; n < cnt; n++)
      {
	Collection *extent;
	classes[n]->getExtent(extent);
	if (coll_oid == extent->getOidC())
	  {
	    if (n < cnt - 1)
	      {
		classes[n+1]->getExtent(_coll);
		return True;
	      }
	  }
      }

    return False;
  }

  Bool get_next_coll(Collection *&_coll) {
    if (get_next_coll_realize(cls, _coll))
      {
	start = 0;
	return True;
      }

    ok = True;
    _coll = 0;
    return False;
  }
};

static const char *
make_ref()
{
  if (idbW_ctx->etc_cnt == 1) {
    Oid oid(idbW_ctx->etc[0]);
    if (!oid.isValid())
      return "  ";
  }

  Bool is_expanded;
  std::string url = idbW_ctx->exp_ctx.build(idbW_ctx->dump_ctx, is_expanded);
  char *str = idbWNewTagBuf(strlen(url.c_str())+1024);

  // try anchors
  sprintf(str, "<a name=\"a%d\"><a href=\"http://%s/%s?"
	  "-ck"      idbWSPACE "%s" idbWSPACE
	  "-%s" idbWSPACE "%s" idbWSPACE
	  "-db"      idbWSPACE "%s#a%d\">%s</a> ",
	  idbW_ctx->anchor,
	  idbW_ctx->cgidir, idbW_progname, idbW_ctx->cookie,
	  "dumpgen",
	  url.c_str(),
	  idbW_ctx->dbname,
	  idbW_ctx->anchor,
	  (is_expanded ? idbW_db_config->display_config.close :
	   idbW_db_config->display_config.open));

  idbW_ctx->anchor++;

  /*
  sprintf(str, "<a href=\"http://%s/%s?"
	  "-ck"      idbWSPACE "%s" idbWSPACE
	  "-%s" idbWSPACE "%s" idbWSPACE
	  "-db"      idbWSPACE "%s\">%s</a> ", // 30/09/05: suppress ' '
	  idbW_ctx->cgidir, idbW_progname, idbW_ctx->cookie,
	  "dumpgen",
	  url.c_str(),
	  idbW_ctx->dbname,
	  (is_expanded ? idbW_db_config->display_config.close :
	   idbW_db_config->display_config.open));
  */

  return str;
}

Status
idbWDumpCollection(idbWDest *dest, Collection *coll,
		   const Class *cls, const char *item_name,
		   idbWTextMode textmod, idbWHeaderMode headmod,
		   Bool useTable, idbWDumpCollContext &);
static void
headFlush(Bool &done, idbWDest *dest, const Class *mcl,
	  const char *name, idbWTextMode txtmod,
	  int dims)
{
  if (!done)
    {
      char ichar = idbW_db_config->display_config.indent_char;
      idbW_dest->flush("%c%c%s: ", ichar, ichar,
		       idbWFieldName(mcl, name, txtmod));
      done = True;
    }
  else
    idbW_dest->cflush(", ");
}

static Status
idbWDumpObject(idbWDest *dest, Object *o, idbWTextMode textmod,
	       idbWVerboseMode verbmod,	idbWHeaderMode headmod);
//
// idbWDump
//

Status
idbWDump(idbWDest *dest, Object *o, idbWTextMode textmod)
{
  void *u = o->getUserData();

  if (idbW_ctx->nohtml)
    textmod = idbWTextSimple;

  idbW_ctx->glb_textmod = textmod;
  
  idbW_status = idbWDumpObject(dest, o, textmod, idbW_ctx->verbmod,
			       idbWHeaderYes);

  o->setUserData(u);
  return idbW_status;
}

static int
idbWDumpGen(Object *o)
{
  idbW_ctx->dump_ctx.init(o);
  idbW_CHECK(idbWDumpObject(idbW_dest, o, idbWTextHTML,
			    idbW_ctx->verbmod, idbWHeaderYes));
  return 0;
}

extern void
get_prefix(const Object *agr, const Class *cls_owner,
	       char prefix[], int len);

Status
idbWDumpEnumClass(idbWDest *dest, EnumClass *cls,
		  idbWTextMode textmod,
		  idbWHeaderMode headmod)
{
  int len;

  if (headmod == idbWHeaderYes)
    idbW_CHECK_FLUSH(idbW_dest->flush("enum <strong>%s</strong>\n", cls->getName()));

  idbW_CHECK_FLUSH(idbW_dest->flush("<strong>{</strong>\n"));
  idbW_dest->push();

  int items_cnt;
  const EnumItem **items;

  items = cls->getEnumItems(items_cnt);
  Object *o = cls->newObj();

  for (int i = 0; i < items_cnt; i++)
    {
      const EnumItem *item = items[i];
      if (i)
	idbW_NL();
      idbW_dest->flush("%s = %d%s", item->getName(), item->getValue(),
		  ((i != items_cnt - 1) ? "," : ""));
    }

  idbW_dest->pop();
  idbW_NL();
  idbW_CHECK_FLUSH(idbW_dest->flush("<strong>}</strong>\n"));
  o->release();

  return Success;
}

static int
dbhead(Database *db)
{
  if (db->getDbid() != idbW_ctx->db->getDbid())
    {
      if (idbWOpenCrossDB(db->getName()))
	return 1;
    }
  
  return 0;
}

Status
idbWDumpAgregatClass(idbWDest *dest, Class *cls, 
		     idbWTextMode textmod,
		     idbWHeaderMode headmod, Bool intable)
{
  int len;

  if (headmod == idbWHeaderYes)
    {
      if (dbhead(cls->getDatabase()))
	return Success;
      cls->wholeComplete();
	
      idbW_CHECK_FLUSH(idbW_dest->flush("class <strong>%s</strong>", cls->getName()));
      Class *parent = cls->getParent();
      if (parent && parent->asAgregatClass())
	{
	  const char *tag = parent->getName();
	  tag = idbWMakeTag(parent->getOid(), tag, textmod, len, True,
			     True);
	  idbW_CHECK_FLUSH(idbW_dest->cflush(" extends %s  ", tag));
	}

      idbW_CHECK_FLUSH(idbW_NL());
    }

  idbW_CHECK_FLUSH(idbW_dest->flush("<strong>{</strong>"));
  idbW_CHECK_FLUSH(idbW_NL());
  idbW_dest->push();

  unsigned int items_cnt;
  const Attribute **items;

  items = cls->getAttributes(items_cnt);
  Object *o = cls->newObj();

  for (int i = 0; i < items_cnt; i++)
    {
      const Attribute *item = items[i];
      if (item->isNative() || !item->getClassOwner()->compare(cls))
	continue;

      char prefix[128];
      eyedb::get_prefix(o, item->getClassOwner(), prefix, sizeof(prefix));

      if (textmod == idbWTextHTML) {
	const char *tag = strdup(idbWMakeHTMLTag(item->getClass()->getName()));
	char *tofree = (char *)tag;

	int len;
	if ((item->getClass()->asAgregatClass() ||
	     item->getClass()->asEnumClass()) &&
	    !item->getClass()->isSystem())
	  tag = idbWMakeTag(item->getClass()->getOid(), tag,
			    textmod, len, True, True);
	else if (item->getClass()->asCollectionClass())
	  tag = idbWMakeTag(item->getClass()->asCollectionClass()->
			    getCollClass()->getOid(),
			    tag, textmod, len, True, True);
	
	if (item->isString())
	  idbW_dest->flush("<strong>attribute</strong> string %s%s", 
			   prefix, item->getName());
	else
	  idbW_dest->flush("<strong>%s</strong> %s %s%s%s", 
			   (item->hasInverse() ? "relationship" : "attribute"),
			   tag, item->isIndirect() ? "*" : "",
			   prefix, item->getName());
	
	free(tofree);
      }
      else
	idbW_dest->flush("%s %s%s%s", 
			 item->getClass()->getName(),
			 item->isIndirect() ? "*" : "",
			 prefix, item->getName());
      
      if (!item->isString()) {
	const TypeModifier typmod = item->getTypeModifier();
	int i;
	for (i = 0; i < typmod.ndims; i++) {
	  if (typmod.dims[i] > 0)
	    idbW_dest->cflush("[%d]", typmod.dims[i]);
	  else
	    idbW_dest->cflush("[]", typmod.dims[i]);
	}
      }
      
      idbW_dest->cflush(" ");
      idbW_NL();
    }

  const LinkedList *mlist = cls->getCompList();
  LinkedListCursor cc(mlist);
  Object *co;
  while (cc.getNext((void *&)co))
    {
      const char *tag;
      int len;
      idbW_CHECK_S(idbWGetTag(co, tag, textmod, len));
      if (strcmp(tag, idbW_skipTag))
	idbW_dest->flush("%s\n", tag);
    }

  // reference to collobjs
  idbW_dest->pop();
  idbW_CHECK_FLUSH(idbW_dest->flush("<strong>}</strong>"));
  if (!intable) idbW_NL();

  o->release();
  return Success;
}

Status
idbWDumpSchema(idbWDest *dest, Schema *m,
		     idbWTextMode textmod,
		     idbWHeaderMode headmod)
{
  m->complete(True, True);

  const LinkedList *list = m->getClassList();
  LinkedListCursor c(list);

  Class *mcl;

  while (list->getNextObject(&c, (void *&)mcl))
    if (!mcl->asCollectionClass() && !mcl->isSystem())
      idbWDumpAgregatClass(dest, mcl, textmod, headmod);

  return Success;
}

Status
idbWDumpImage(idbWDest *dest, Image *image)
{
  if (!idbW_ctx->imgdir)
    return Exception::make(IDB_ERROR, "EyeDB web server is not "
			      "configured for image display: missing image directory");
  if (!idbW_ctx->rootdir)
    return Exception::make(IDB_ERROR, "EyeDB web server is not "
			      "configured for image display: missing root directory");

  const char *extension = 0;
  ImageType::Type im_type = image->getType();

  if (im_type == ImageType::JPEG_IMG_TYPE)
    extension = ".jpg";
  else if (im_type == ImageType::GIF_IMG_TYPE)
    extension = ".gif";
  
  if (!extension)
    return Success;

  const char *tmpfile = idbW_get_tmp_file(extension, -1);
  int fd = creat((std::string(idbW_ctx->rootdir) + "/" + tmpfile).c_str(), 0666);
  if (fd < 0)
    return Exception::make(IDB_ERROR, "cannot create file '%s'",
			      tmpfile);

  fchmod(fd, 0666);

  int data_cnt = image->getDataCount();
  char *buf = 0;
  image->getClass()->getAttribute("data")->
    getValue(image, (Data *)&buf, Attribute::directAccess, 0);
  write(fd, buf, data_cnt);
  close(fd);

  idbW_dest->flush("<img src=\"http://%s/%s\">", idbW_ctx->als, tmpfile);
  return Success;
}

static Status
idbWItemDump(idbWDest *dest, Object *o,
	     const idbWDisplayConfig::Config::Attr *xitem,
	     idbWTextMode textmod, idbWVerboseMode verbmod,
	     idbWHeaderMode headmod);

static Status
idbWItemDumpExtra(idbWDest *dest, Object *o,
		  const idbWDisplayConfig::Config::Attr *item,
		  idbWTextMode textmod, idbWVerboseMode verbmod,
		  idbWHeaderMode headmod);

Status
idbWDumpObjectRealize(idbWDest *dest, Object *o, idbWTextMode textmod,
		      idbWVerboseMode verbmod, idbWHeaderMode headmod)
{
  idbW_dest->push();

  int items_cnt;
  const idbWDisplayConfig::Config::Attr *items;

  items = idbW_getAttrList(o->getClass(), items_cnt);

  for (int n = 0; n < items_cnt; n++)
    if (items[n].isIn)
      idbW_CHECK_S(idbWItemDump(dest, o, &items[n], textmod, verbmod, headmod));

  /*
  const Attribute **items;

  items = o->getClass()->getAttributes(items_cnt);

  for (int n = 0; n < items_cnt; n++)
    idbW_CHECK_S(idbWItemDump(dest, o, items[n], textmod, verbmod, headmod));
    */

  idbW_dest->pop();
}

static Status
idbWDumpObject(idbWDest *dest, Object *o, idbWTextMode textmod,
	       idbWVerboseMode verbmod,idbWHeaderMode headmod)
{
  idbWObjectContext *obj_ctx;

  if (obj_ctx = (idbWObjectContext *)o->getUserData())
    {
      if (obj_ctx->tstamp != idbW_tstamp)
	{
	  delete obj_ctx;
	  o->setUserData((void *)0);
	}
      else
	return Success;
    }

  o->setUserData(new idbWObjectContext());

  Status s;

  if (o->asAgregatClass())
    {
      s = idbWDumpAgregatClass(dest, (AgregatClass *)o, textmod,
				headmod);
      return s;
    }

  if (o->asEnumClass())
    {
      s = idbWDumpEnumClass(dest, (EnumClass *)o, textmod,
			     headmod);
      return s;
    }

  if (o->getClass()->asCollectionClass())
    {
      const char *name = ((Collection *)o)->getName();
      if (!name || !*name)
	name = "<unnamed>";
      Collection *coll = (Collection *)o;
      idbWDumpCollContext ctx(0, coll, 0, name);
      int done = 0;
      s = idbWDumpCollection(idbW_dest, coll,
			     o->getClass(),
			     name,
			     textmod, headmod, True, ctx);
      if (done)
	idbW_dest->pop();
      return s;
    }

  if (o->getDatabase() &&
      o->getOid().compare(o->getDatabase()->getSchema()->getOid()))
    {
      s = idbWDumpSchema(dest, (Schema *)o, textmod, headmod);

      return s;
    }

  const Class *cls = o->getClass();

  int len;
  if (headmod == idbWHeaderYes)
    {
      const char *tag;
      const char *fnm = idbWFieldName(cls, cls->getName(), textmod, True);

      //      if (/*dump_ctx == idbWRecursNone || */idbW_dest->isTopLevel())
      if (1)
	idbWGetTag(o, tag, textmod, len);
      else
	tag = idbW_null;

      if (!tag) tag = idbW_null;

      if (dbhead(o->getDatabase()))
	return Success;

      idbW_CHECK_FLUSH(idbW_dest->flush("%s: %s  \n", fnm, tag));
    }

   if (!strcmp(cls->getName(), "image") && textmod == idbWTextHTML)
    return idbWDumpImage(dest, (Image *)o);

  Bool found;
  idbW_status = idbWOverLoadDump(o, found);
  if (!idbW_status && found)
    {
      if (!idbW_status && headmod == idbWHeaderYes)
	idbW_CHECK_FLUSH(idbW_NL());
      return idbW_status;
    }

  if (idbW_ctx->exp_ctx.isExpanded(idbW_ctx->dump_ctx))
    idbWDumpObjectRealize(dest, o, textmod, verbmod, headmod);

  delete (idbWObjectContext *)(o->getUserData());
  o->setUserData(0); // added the 9/10/99
  return Success;
}

#define NO_CLASS_DETAILS

Status
idbWGetTagMethod(const Class *cls, Object *o,
		  const char *&tag, idbWTextMode textmod, int &len)
{
  
  Method *m = (Method *)o;
  const Executable *ex = m->getEx();
  const Signature *sign = ex->getSign();

#ifdef NO_CLASS_DETAILS
  tag = idbWNewTagBuf();
  strcpy((char *)tag, idbW_skipTag);
  if (ex->getLang() & SYSTEM_EXEC)
    return Success;
#endif

  char *str = idbWNewTagBuf();
  char *str_tag = idbWNewTagBuf();

  sprintf(str_tag, "&lt;%s&gt; %s %s::%s(",
	  (m->asFEMethod_C() ? "frontend" : "backend"),
	  Argument::getArgTypeStr(sign->getRettype()),
	  m->getClassOwner()->getName(),
	  ex->getExname().c_str());

  char tok[64];

  int n = sign->getNargs();
  for (int i = 0; i < n; i++)
    {
      sprintf(tok, "%s%s", (i ? ", " : ""),
	      Argument::getArgTypeStr(sign->getTypes(i)));
      strcat(str_tag, tok);
    }

  strcat(str_tag, ")");

  str_tag = idbWMakeTag(o->getOid(), str_tag, textmod, len);

  sprintf(str, "<strong>method</strong> %s", str_tag);

  tag = str;

  return Success;
}

Status
idbWGetTagUniqueConstraint(const Class *cls, Object *o,
			    const char *&tag, idbWTextMode textmod, int &len)
{
  
#ifdef NO_CLASS_DETAILS
  tag = idbWNewTagBuf();
  strcpy((char *)tag, idbW_skipTag);
  return Success;
#endif
  UniqueConstraint *u = (UniqueConstraint *)o;
  char *str = idbWNewTagBuf();
  char *str_tag = idbWNewTagBuf();


  sprintf(str_tag, "unique(%s::%s)",
	  //(u->getIscomp() ? "[]" : ""),
	  u->getClassOwner()->getName(), u->getName().c_str());

  str_tag = idbWMakeTag(o->getOid(), str_tag, textmod, len);

  sprintf(str, "<strong>constraint</strong> %s", str_tag);

  tag = str;
  return Success;
}

Status
idbWGetTagIndex(const Class *cls, Object *o,
		 const char *&tag, idbWTextMode textmod, int &len)
{
#ifdef NO_CLASS_DETAILS
  tag = idbWNewTagBuf();
  strcpy((char *)tag, idbW_skipTag);
  return Success;
#endif
  Index *u = (Index *)o;
  char *str = idbWNewTagBuf();
  char *str_tag = idbWNewTagBuf();

  strcpy(str_tag, u->getName().c_str());

  str_tag = idbWMakeTag(o->getOid(), str_tag, textmod, len);

  sprintf(str, "<strong>index</strong> %s", str_tag);

  tag = str;
  return Success;
}

Status
idbWGetTagNotNullConstraint(const Class *cls, Object *o,
			     const char *&tag, idbWTextMode textmod, int &len)
{
#ifdef NO_CLASS_DETAILS
  tag = idbWNewTagBuf();
  strcpy((char *)tag, idbW_skipTag);
  return Success;
#endif
  NotNullConstraint *u = (NotNullConstraint *)o;
  char *str = idbWNewTagBuf();
  char *str_tag = idbWNewTagBuf();

  sprintf(str_tag, "notnull(%s::%s)",
	  //(u->getIscomp() ? "[]" : ""),
	  u->getClassOwner()->getName(), u->getName().c_str());
  
  str_tag = idbWMakeTag(o->getOid(), str_tag, textmod, len);

  sprintf(str, "<strong>constraint</strong> %s", str_tag);

  tag = str;
  return Success;
}

Status
idbWGetTagCardinalityConstraint(const Class *cls, Object *o,
				 const char *&tag, idbWTextMode textmod, int &len)
{
#ifdef NO_CLASS_DETAILS
  tag = idbWNewTagBuf();
  strcpy((char *)tag, idbW_skipTag);
  return Success;
#endif
  CardinalityConstraint *card = (CardinalityConstraint *)o;
  char *str = idbWNewTagBuf();
  char *str_tag = idbWNewTagBuf();

  sprintf(str_tag, "card (%s::%s) %s",
	  card->getClassOwner()->getName(), card->getName().c_str(),
	  card->getCardDesc()->getString(False));
  
  str_tag = idbWMakeTag(o->getOid(), str_tag, textmod, len);

  sprintf(str, "<strong>constraint</strong> %s", str_tag);

  tag = str;
  return Success;
}

Status
idbWGetTagClassVariable(const Class *cls, Object *o,
			     const char *&tag, idbWTextMode textmod, int &len)
{
#ifdef NO_CLASS_DETAILS
  tag = idbWNewTagBuf();
  strcpy((char *)tag, idbW_skipTag);
  return Success;
#endif
  ClassVariable *v = (ClassVariable *)o;

  char *str = idbWNewTagBuf();
  char *str_tag = idbWNewTagBuf();

  sprintf(str_tag, "%s::%s",
	  (v->getClassOwner() ? v->getClassOwner()->getName() :
	   "??"), v->getVname().c_str());

  str_tag = idbWMakeTag(o->getOid(), str_tag, textmod, len);

  sprintf(str, "<strong>variable</strong>   %s", str_tag);

  tag = str;
  return Success;
}

Status
idbWGetTagTrigger(const Class *cls, Object *o,
		   const char *&tag, idbWTextMode textmod, int &len)
{
#ifdef NO_CLASS_DETAILS
  tag = idbWNewTagBuf();
  strcpy((char *)tag, idbW_skipTag);
  return Success;
#endif
  return Success;
}

static Status
idbWGetTagPerform(Object *o, const char *&tag, idbWTextMode textmod,
		  int &len);

Status
idbWGetTagRealize(Object *o, const char *&tag, idbWTextMode textmod, int &len)
{
  if (o->asClass())
    {
      tag = idbWMakeTag(o->getOid(), ((Class *)o)->getName(),
			 textmod, len);
      return Success;
    }

  const Class *cls = o->getClass();
  const char *mcl_name = cls->getName();

  tag = 0;

  if (cls->asEnumClass())
    idbW_status = idbWGetTagEnum((EnumClass *)cls, (Enum *)o, tag,
			     textmod, len);
  else if (!strcmp(mcl_name, "be_method_C") ||
	   !strcmp(mcl_name, "fe_method_C"))
    idbW_status = idbWGetTagMethod(cls, o, tag, textmod, len);
  else if (!strcmp(mcl_name, "hashindex") ||
	   !strcmp(mcl_name, "btreeindex"))
    idbW_status = idbWGetTagIndex(cls, o, tag, textmod, len);
  else if (!strcmp(mcl_name, "unique_constraint"))
    idbW_status = idbWGetTagUniqueConstraint(cls, o, tag, textmod, len);
  else if (!strcmp(mcl_name, "cardinality_constraint"))
    idbW_status = idbWGetTagCardinalityConstraint(cls, o, tag, textmod, len);
  else if (!strcmp(mcl_name, "notnull_constraint"))
    idbW_status = idbWGetTagNotNullConstraint(cls, o, tag, textmod, len);
  else if (!strcmp(mcl_name, "trigger"))
    idbW_status = idbWGetTagTrigger(cls, o, tag, textmod, len);
  else if (!strcmp(mcl_name, "class_variable"))
    idbW_status = idbWGetTagClassVariable(cls, o, tag, textmod, len);
  else
    idbW_status = idbWGetTagClass(cls, o, tag, textmod, len);

  /*
  if (!idbW_status && (!tag || !*tag))
    tag = idbWMakeTag(o->getOid(), idbW_unknownTag, textmod, len);
    */

  return idbW_status;
}

Status
idbWGetTag(Object *o, const char *&tag, idbWTextMode textmod, int &len)
{
  Status s = idbWGetTagPerform(o, tag, textmod, len);
  if (s) return s;
  if ((!tag || !*tag))
    tag = idbWMakeTag(o->getOid(), idbW_unknownTag, textmod, len);
  return Success;
}

static Status
idbWOQLExec(Object *o, const char *oql, char *&str)
{
  /*
  idbOQL q(o->getDatabase(), std::string("(this := ") +
	   o->getOid().toString() + ", (" + oql + "))");
	   */
  OQL q(o->getDatabase(), (std::string("this := ") +
	   o->getOid().toString() + "; " + oql).c_str());
  ValueArray val_arr;
  idbW_status = q.execute(val_arr);
  if (!idbW_status &&
      (val_arr.getCount() != 1 || val_arr[0].type != Value::tString))
    idbW_status =
      Exception::make("invalid result using the tag function"
			 "'%s'", oql);

  if (idbW_status)
    {
      idbW_STPR(idbW_status);
      return idbW_status;
    }

  str = strdup(val_arr[0].str);
  return Success;
}

Status
idbWGetTagPerform(Object *o, const char *&tag, idbWTextMode textmod, int &len)
{
  if (!o)
    {
      tag = "????";
      return Success;
    }

  Method *mth = getWGetTagMethod(o);

  if (!mth)
    {
      const char *oql = idbW_getGetTagOQL(o);
      if (!oql)
	return idbWGetTagRealize(o, tag, textmod, len);

      char *str = 0;
      if (idbW_status = idbWOQLExec(o, oql, str))
	return idbW_status;

      len = strlen(str);
      tag = idbWMakeTag(o->getOid(), str, textmod, len);
      free(str);
      return Success;
    }


  ArgArray array;
  Argument retarg;

  idbW_status = mth->applyTo(o->getDatabase(), o, array, retarg, False);

  if (idbW_status)
    {
      idbW_STPR(idbW_status);
      // changed the 2/2/99
      return idbWGetTagRealize(o, tag, textmod, len);
    }

  len = strlen(retarg.u.s);
  tag = idbWMakeTag(o->getOid(), retarg.u.s, textmod, len);

  return Success;
}

Status
idbWGetTagEnum(const EnumClass *menum, Enum *o, const char *&tag,
		idbWTextMode textmod, int &len)
{
  static const char unknown[] = idbW_null;
  static int len_unknown = strlen(unknown);

  unsigned int v;

  idbW_CHECK_S(o->getValue(&v));

  /*
  if (!v)
    {
      tag = idbWMakeTag(o->getOid(), unknown, textmod, len);
      len = len_unknown;
      return Success;
    }
    */

  const EnumItem *enit = menum->getEnumItemFromVal(v);

  if (enit)
    {
      len = strlen(tag);
      tag = idbWMakeTag(o->getOid(), enit->getName(), textmod, len);
    }
  else
    {
      len = len_unknown;
      tag = idbWMakeTag(o->getOid(), unknown, textmod, len);
    }

  return Success;
}

#define MAXTAGLEN 80

static Bool
idbWMakeTag(const Attribute *item, Object *o, const char *&tag,
	     idbWTextMode textmod, int &len)
{
  tag = idbWGetString(item, o);

  if (tag && *tag)
    {
      char *p = 0;
      if (strlen(tag) > MAXTAGLEN)
	{
	  p = (char *)malloc((MAXTAGLEN*sizeof(char))+1);
	  char *q = p, c;
	  int len = 0;
	  while ((c = *tag++) && (len++ < MAXTAGLEN - 6))
	    {
	      if (c != '\n')
		*q++ = c;
	    }

	  *q = 0;
	  strcat(q, " ...");
	  tag = p;
	}

      tag = idbWMakeTag(o->getOid(), tag, textmod, len);

      if (tag != p)
	free(p);

      return True;
    }

  len = 0;
  return False;
}

static void
idbWBuildFirstUniqueItem(const Class *cls)
{
  idbWObjectContext *obj_ctx;

  if (!(obj_ctx = (idbWObjectContext *)cls->getUserData()) ||
      obj_ctx->tstamp != idbW_tstamp)
    {
      delete obj_ctx;
      obj_ctx = new idbWObjectContext();
      ((Class *)cls)->setUserData(obj_ctx);
    }
  else if (obj_ctx->unique_compute)
    return;

  obj_ctx->unique_compute = True;

  // forget unique facility
#if 1
  obj_ctx->unique = False;
  return;
#else
  const LinkedList *uniq_list =
    cls->getCompList(Class::idbUniqueLIST);

  if (!uniq_list)
    {
      obj_ctx->unique = False;
      return;
    }

  const Attribute *chosen = 0;
  LinkedListCursor c(uniq_list);
  idbUniqueConstraint *u;

  while (uniq_list->getNextObject(&c, (void *&)u))
    {
      if (!u->getIscomp())
	continue;

      const Attribute *item = cls->getAttribute(u->getAttrname());
      if (idbWIsString(item) &&
	  (!chosen || item->getNum() < chosen->getNum()))
	chosen = item;
    }

  if (chosen)
    {
      obj_ctx->unique = True;
      obj_ctx->item = chosen;
    }
  else
    obj_ctx->unique = False;
#endif
}

Status
idbWGetTagClass(const Class *cls, Object *o, const char *&tag,
		 idbWTextMode textmod, int &len)
{
  unsigned int items_cnt, n;
  const Attribute **items = cls->getAttributes(items_cnt);
  idbWObjectContext *obj_ctx;

  idbWBuildFirstUniqueItem(cls);

  if (obj_ctx = (idbWObjectContext *)cls->getUserData())
    {
      if (obj_ctx->tstamp != idbW_tstamp)
	{
	  delete obj_ctx;
	  ((Class *)cls)->setUserData((void *)0);
	}
      else if (obj_ctx->unique &&
	       idbWMakeTag(obj_ctx->item, o, tag, textmod, len))
	return Success;
    }

  for (n = 0; n < items_cnt; n++)
    {
      const Attribute *item = items[n];

      if (idbWIsString(item))
	{
	  Bool isnull = False;
	  tag = idbWGetString(item, o);
	  if (isnull || !tag || !*tag) continue;

	  if (idbWMakeTag(item, o, tag, textmod, len))
	    return Success;
	}
      else if (!item->getClass()->asBasicClass() && !item->isIndirect())
	{
	  idbW_CHECK_S(idbWGetTagItem(item, o, tag, textmod, len));
	  if (tag && *tag)
	    return Success;
	}
    }

  return Success;
}

const char *
idbWGetTag(Object *o)
{
  if (!o)
    return "?";

  int len;
  const char *tag = "";
  idbWGetTag(o, tag, idbWTextSimple, len);
  return tag;
}

Status
idbWGetTagItem(const Attribute *item, Object *o, const char *&tag,
		idbWTextMode textmod, int &len)
{
  Object *po;
  int ndims = idbWGetSize(item, o);

  for (int j = 0; j < ndims; j++)
    {
      if (item->getClass()->asEnumClass())
	{
	  int val;
	  const char *enname = idbWGetEnumName(item, o, j, &val);
	  if (enname && *enname)
	    {
	      tag = idbWMakeTag(o->getOid(), enname, textmod, len);
	      return Success;
	    }
	}
      else
	{
	  idbW_CHECK_S(item->getValue(o, (Data *)&po, 1, j));

	  if (po)
	    {
	      idbW_CHECK_S(idbWGetTagPerform(po, tag, idbWTextSimple, len));
	      
	      if (tag && *tag)
		{
		  tag = idbWMakeTag(o->getOid(), tag, textmod, len);
		  return Success;
		}
	    }
	}
    }

  return Success;
}

//
// Tag utilities
//

static const int ntagbuf = 64;

char *
idbWNewTagBuf(int len)
{
  static char *tagbuf[ntagbuf];
  static int tagbuf_len[ntagbuf];
  static int which;

  if (which >= ntagbuf)
    which = 0;

  if (len >= tagbuf_len[which])
    {
      tagbuf_len[which] = len + 2048;
      free(tagbuf[which]);
      tagbuf[which] = (char *)malloc(tagbuf_len[which]);
    }

  return tagbuf[which++];
}

static void
append(char *&q, const char *s)
{
  *q = 0;
  strcat(q, s);
  q += strlen(s);
}

const char *
idbWMakeHTMLTag(const char *tag)
{
  static char buf[2048];
  const char *p = tag;
  char *q = buf, c;

  *q = 0;

  while (c = *p)
    {
      if (c == '<')
	append(q, "&lt;");
      else if (c == '>')
	append(q, "&gt;");
      else
	*q++ = *p;

      p++;
    }

  *q = 0;
  return buf;
}

char *
idbWMakeTag(const Oid &oid, const char * tag, idbWTextMode textmod,
	    int &len, Bool strong, Bool reqdlggen)
{
  len = strlen(tag);

  char *str = idbWNewTagBuf(len);

  if (textmod != idbWTextHTML)
    {
      sprintf(str, "%s%s%s", // was "  "
	      (strong ? "<em><strong>" : ""), idbWMakeHTMLTag(tag),
	      (strong ? "</strong></em>" : ""));
      return str;
    }

  if (!oid.isValid())
    {
      if (!idbW_ctx->exp_ctx.isExpanded(idbW_ctx->dump_ctx))
	sprintf(str, "%s%s%s",
	      (strong ? "<em><strong>" : ""), idbWMakeHTMLTag(tag),
	      (strong ? "</strong></em>" : ""));
      else
	*str = 0;
    }
  else
    {
      if (reqdlggen)
	sprintf(str, "<a href=\"http://%s/%s?"
		"-ck"      idbWSPACE "%s" idbWSPACE
		"-%s" idbWSPACE "%s" idbWSPACE
		"-db"      idbWSPACE "%s\">%s%s%s</a>",
		idbW_ctx->cgidir, idbW_progname, idbW_ctx->cookie,
		"reqdlggen",
		oid.getString(), idbW_ctx->dbname,
		(strong ? "<em><strong>" : ""),
		idbWMakeHTMLTag(tag), (strong ? "</strong></em>" : ""));
      else
	{
	  sprintf(str, "<a href=\"http://%s/%s?"
		  "-ck"      idbWSPACE "%s" idbWSPACE
		  "-%s" idbWSPACE "%s" idbWSPACE
		  "-db"      idbWSPACE "%s\">%s%s%s</a>",
		  idbW_ctx->cgidir, idbW_progname, idbW_ctx->cookie,
		  "dumpgen",
		  oid.getString(), idbW_ctx->dbname,
		  (strong ? "<em><strong>" : ""),
		  idbWMakeHTMLTag(tag), (strong ? "</strong></em>" : ""));
	}
    }

  return str;
}

const char *
idbWGetEnumName(const Attribute *item, Object *o, int from, int *val)
{
  const EnumItem *enit;

  eyedb_CHECK(item->getValue(o, (Data *)val, 1, from));
  enit = ((EnumClass * )item->getClass())->getEnumItemFromVal(*val);

  if (enit)
    return enit->getName();

  return "";
}

const char *
idbWFieldName(const Class *cls, const char *name,
	       idbWTextMode textmod, Bool head)
{
  if (!head && !idbW_db_config->display_config.type_link)
    {
      char *str = idbWNewTagBuf(strlen(name)+100);
      //      sprintf(str, "<em><strong>%s</strong></em>", name);
      //sprintf(str, "<b>%s</b>", name);
      //      sprintf(str, "<span style=\"text-decoration: underline; font-weigth: bold;\">%s</span>", name);
      // TBD: should be configurable !!!
      sprintf(str, "<span style=\"color: %s; font-weigth: bold;\">%s</span>", idbW_db_config->look_config.fieldcolor, name);
      return str;
    }
      
  static Oid null_oid;
  int len;
  if (!cls->asAgregatClass())
    return idbWMakeTag((cls ? cls->getOid() : null_oid), name,
			idbWTextSimple, len, True);
  return idbWMakeTag((cls ? cls->getOid() : null_oid), name,
		      textmod, len, True, True);
}

static Status
idbWItemDump(idbWDest *dest, Object *o,
	     const idbWDisplayConfig::Config::Attr *xitem,
	     idbWTextMode textmod, idbWVerboseMode verbmod,
	     idbWHeaderMode headmod)
{
  if (xitem->isExtra)
    return idbWItemDumpExtra(dest, o, xitem, textmod, verbmod, headmod);

  const Attribute *item = xitem->attr;

  if (item->isNative() && !o->asClass())
    return Success;
  
  const Class *cls = item->getClass();

  if (cls->asCollectionClass())
    return idbWItemDumpCollection(dest, o, item, textmod,
				  verbmod, headmod);

  if (item->isIndirect() || cls->asAgregatClass())
    return idbWItemDumpObject(dest, o, item, textmod,
			      verbmod, headmod);

  if (cls->asBasicClass())
    return idbWItemDumpBasic(dest, o, item, textmod,
			     verbmod, headmod);
  
  if (cls->asEnumClass())
    return idbWItemDumpEnum(dest, o, item, textmod,
			     verbmod, headmod);

  fprintf(stderr, "unknown type for `%s'\n", cls->getName());

  return Success;
}

#define MAXPRES 80
#define MINPRES  6

static void
idbWPrintFloat(double d)
{
  double od = 0;
  char tok[128], fmt[32];

  for (int i = MINPRES; i < MAXPRES; i += 4)
    {
      sprintf(fmt, "%%.%dg", i);
      sprintf(tok, fmt, d);
      double cd = atof(tok);
      if (cd == d || (i != MINPRES && od != cd))
	{
	  idbW_dest->cflush(tok);
	  return;
	}

      od = cd;
    }

  sprintf(fmt, "%%.%dg", MAXPRES);
  sprintf(tok, fmt, d);
  idbW_dest->cflush(tok);
}

Status
idbWItemDumpBasic(idbWDest *dest, Object *o, const Attribute *item,
		  idbWTextMode textmod, idbWVerboseMode verbmod,
		  idbWHeaderMode headmod)
{
  const Class *mcl = item->getClass();
  TypeModifier tmod = item->getTypeModifier();
  const char *name = item->getName();
  Bool verbfull = (verbmod == idbWVerboseFull) ? True : False;
  Bool done = False;

  int dims = idbWGetSize(item, o);

  if (idbWIsString(item))
    {
      Bool isnull;
      char *s = idbWGetString(item, o, &isnull);
      if (!isnull || verbfull)
	{
	  char ichar = idbW_db_config->display_config.indent_char;
	  idbW_CHECK_FLUSH(idbW_dest->flush("%c%c%s: ",
					    ichar, ichar,
					    idbWFieldName(item->getClass(),
							  name, textmod)));
	  idbWFlushText(dest, (s ? s : ""), False, isnull);
	  idbW_dest->cflush("  ");
	  idbW_NL();
	}

      return Success;
    }

  for (int i = 0; i < dims; i++)
    {
      Bool isnull;
      if (idbWIsint16(item))
	{
	  eyedblib::int16 val;
	  val = idbWGetInt16(item, o, i, &isnull);
	  if (!isnull || verbfull)
	    {
	      headFlush(done, dest, item->getClass(), name, textmod, dims);
	      if (isnull)
		idbW_dest->cflush(idbW_null);
	      else
		idbW_dest->cflush("%d", val);
	    }
	}
      else if (idbWIsint32(item))
	{
	  eyedblib::int32 val;
	  val = idbWGetInt32(item, o, i, &isnull);
	  if (!isnull || verbfull)
	    {
	      headFlush(done, dest, item->getClass(), name, textmod, dims);
	      if (isnull)
		idbW_dest->cflush(idbW_null);
	      else
		idbW_dest->cflush("%d", val);
	    }
	}
      else if (idbWIsint64(item))
	{
	  eyedblib::int64 val;
	  val = idbWGetInt64(item, o, i, &isnull);
	  if (!isnull || verbfull)
	    {
	      headFlush(done, dest, item->getClass(), name, textmod, dims);
	      if (isnull)
		idbW_dest->cflush(idbW_null);
	      else
		idbW_dest->cflush("%lld", val);
	    }
	}
      else if (idbWIsFloat(item))
	{
	  double fval;
	  
	  fval = idbWGetFloat(item, o, i, &isnull);
	  if (!isnull || verbfull)
	    {
	      headFlush(done, dest, item->getClass(), name, textmod, dims);
	      if (isnull)
		idbW_dest->cflush(idbW_null);
	      else
		idbWPrintFloat(fval);
	    }
	}
      else if (idbWIsChar(item))
	{
	  char val;
	  val = idbWGetChar(item, o, i, &isnull);
	  if (!isnull || verbfull)
	    {
	      headFlush(done, dest, item->getClass(), name, textmod, dims);
	      if (isnull)
		idbW_dest->cflush(idbW_null);
	      else
		idbW_dest->cflush("%c", val);
	    }
	}
    }

  if (done)
    {
      idbW_dest->cflush("  ");
      idbW_NL();
    }

  return Success;
}

static Status
idbWItemDumpExtra(idbWDest *dest, Object *o,
		  const idbWDisplayConfig::Config::Attr *item,
		  idbWTextMode textmod, idbWVerboseMode verbmod,
		  idbWHeaderMode headmod)
{
  char ichar = idbW_db_config->display_config.indent_char;
  idbW_dest->flush("%c%c", ichar, ichar);
  if (*item->getName())
    idbW_dest->cflush("<em><strong>%s</strong></em>: ", item->getName());

  char *str = 0;
  if (idbW_status = idbWOQLExec(o, item->extra.oql, str))
    return idbW_status;

  idbW_dest->cflush(str);
  free(str);
  idbW_NL();
  return Success;
}

static Status
autoLoad(const Attribute *item, Object *o, Object **po, int n)
{
  *po = 0;

  idbW_CHECK_S(item->getValue(o, (Data *)po, 1, n));
      
  if (*po && item->isIndirect())
    (*po)->incrRefCount();
  else if (!(*po) && item->isIndirect())
    {
      Oid oid;

      idbW_CHECK_S(item->getOid(o, &oid, 1, n));

      if (oid.isValid())
	idbW_CHECK_S(idbW_ctx->db->loadObject(&oid, po));
    }

  return Success;
}

int
idbW_get_instance_number(const Class *cls, Bool reload)
{
  unsigned int class_cnt;
  Class **classes;
  Status s = cls->getSubClasses(classes, class_cnt);

  if (s) return 0;

  int cnt = 0;
  for (int i = 0; i < class_cnt; i++)
    {
      Collection *extent = 0;
      s = classes[i]->getExtent(extent, reload);
      if (!s && extent)
	cnt += extent->getCount();
    }

  return cnt;
}

int
idbWMakeContinueContext(idbWDest *dest, const char action[],
			const Class *cls, const char *item_name,
			const Oid &colloid, unsigned int start, 
			unsigned int totaldumpedcount,
			idbWTextMode textmod, Bool submit,
			Bool useTable)
{
  if (textmod != idbWTextHTML)
    {
      idbW_dest->flush("CONTINUE\n");
      return 0;
    }

  if (useTable)
    idbW_dest->flush(DG_SCENTER);

#define USE_LINK

  if (submit) {
#ifdef USE_LINK
    if (useTable)
#endif
      idbW_dest->cflush("<strong><form method=POST ACTION=");
  }
  else {
    idbW_dest->flush("<center222><table border=%d>"
		     "<caption align=top><i>To list all the instances</i>"
		     "</caption><tr><th bgcolor=%s><pre>\n",
		     idbW_db_config->look_config.border,
		     idbW_db_config->look_config.tablebgcolor);
    idbW_dest->flush("<strong><em>  <a href=");
  }

#ifdef USE_LINK
  if (useTable)
#endif
  idbW_dest->cflush("\"http://%s/%s?-dump%sgen"
		    idbWSPACE "%s" idbWSPACE "%s" idbWSPACE "%s"
		    idbWSPACE "%d" idbWSPACE "%d" idbWSPACE
		    "-ck" idbWSPACE "%s" idbWSPACE
		    "-db" idbWSPACE "%s" idbWSPACE "\">",
		    idbW_ctx->cgidir, idbW_progname, action,
		    colloid.getString(),
		    (cls ? cls->getOid().getString() : idbW_null),
		    item_name, start,
		    totaldumpedcount,
		    idbW_ctx->cookie,
		    idbW_ctx->dbname);
  
  if (submit)
    {
      if (useTable)
	{
	  idbW_dest->cflush("<input type=\"SUBMIT\" value=\"%s\">"
			    "</form></strong>", action);
	  idbW_dest->flush(DG_ECENTER);
	}
      else // WARNING: the spaces before depends on the indent value
	{
#ifdef USE_LINK
	  idbW_NL();
	  dest->push();
	  dest->push();
	  idbW_dest->flush("<strong><em>  <a href="
			   "\"http://%s/%s?-dump%sgen"
			   idbWSPACE "%s" idbWSPACE "%s" idbWSPACE "%s"
			   idbWSPACE "%d" idbWSPACE "%d" idbWSPACE
			   "-ck" idbWSPACE "%s" idbWSPACE
			   "-db" idbWSPACE "%s" idbWSPACE "%s\">%s</a></strong>",
			   idbW_ctx->cgidir, idbW_progname, action,
			   colloid.getString(),
			   (cls ? cls->getOid().getString() : idbW_null),
			   item_name, start,
			   totaldumpedcount,
			   idbW_ctx->cookie,
			   idbW_ctx->dbname, action, action);
  
	  dest->pop();
	  dest->pop();
	  idbW_NL();
#else
	  idbW_dest->flush("    <input type=\"SUBMIT\" value=\"%s\">"
			   "</form></strong>", action);
#endif
	}
      return 0;
    }

  int cnt = idbW_get_instance_number(cls, True);
  if (cnt)
    idbW_dest->flush("%s instances</a> (%d element%s) </strong></em>",
		     cls->getName(), cnt, (cnt > 1 ? "s" : ""));
  else
    idbW_dest->flush("%s instances</a> (no elements) </strong></em>",
		     cls->getName());
  idbW_dest->flush("\n\n</th></tr></table></center222>\n\n");
  idbW_dest->flush("</center>");
  return cnt;

}

static const char *
getIndent(int i)
{
  static char *indent;

  if (!indent)
    {
      int size = MAXSIZE;
      indent = (char *)malloc(size+1);
      char *p = indent;
      while (size--)
	*p++ = ' ';
      *p = 0;
    }

  return ((i < MAXSIZE) ? &indent[MAXSIZE - i] : indent);
}

static void
idbWDumpCollHead(idbWDest *dest, Collection *, const Class *cls,
		 idbWTextMode textmod, idbWHeaderMode headmod,
		 Bool useTable, Bool isExpanded, idbWDumpCollContext &ctx)
{
  if (headmod == idbWHeaderYes)
    {
      if (dbhead(ctx.db))
	return;
    }
      
  if (useTable)
    idbW_dest->flush("<br>" DG_SCENTER "<table border=%d><caption align=top>"
		     "<em><strong> ", idbW_db_config->display_config.border);
  else
    idbW_dest->flush(make_ref());

  idbW_dest->cflush("%s (<i>%d element%s</i>)",
		   idbWFieldName(cls, ctx.name.c_str(), textmod),
		   ctx.totalcount, idbW_PLURAL(ctx.totalcount));

  if (isExpanded &&
      (ctx.totaldumpedcount + ctx.dumpedcount < ctx.totalcount ||
       ctx.totaldumpedcount))
    idbW_dest->cflush(" from #%d to #%d",
		      ctx.totaldumpedcount+1,
		      ctx.totaldumpedcount+ctx.dumpedcount);

  if (useTable)
    idbW_dest->flush("</caption><tr><th align=left bgcolor=%s><pre>\n",
		     idbW_db_config->look_config.tablebgcolor);
  else
    idbW_dest->cflush("  \n");
}  

#define SPC "    "

static Status
idbWDumpCollBody(idbWDest *dest, Collection *coll, const Class *cls,
		 idbWTextMode textmod, idbWHeaderMode headmod,
		 Bool useTable, idbWDumpCollContext &ctx)
{
  const char **tags = ctx.tags;
  Object **objs = ctx.objs;
  int *lens = ctx.lens;

  if (!useTable)
    {
      dest->push();
      dest->push();
    }

  ctx.maxlen += 2;

  for (int j = 0; j < ctx.dumpedcount; j++)
    {
      if (!lens[j] && !objs[j])
	continue;

      idbW_ctx->dump_ctx.push(coll, idbW_member, j);
      if (!useTable)
	idbW_dest->flush(make_ref());
      else
	idbW_CHECK_FLUSH(idbW_dest->flush(SPC));

      if (objs[j])
	{
	  idbW_CHECK_FLUSH(idbW_dest->cflush(tags[j]));
	  idbW_NL();
	  dest->push();
	  Status s = idbWDumpObject(idbW_dest, objs[j], textmod,
				       idbWVerboseRestricted, idbWHeaderNo);
	  dest->pop();
	  if (s) return s;
	}
      else
	idbW_CHECK_FLUSH(idbW_dest->cflush(tags[j]));

      if (useTable)
	idbW_dest->cflush(SPC);

      idbW_ctx->dump_ctx.pop();
      if (!objs[j] && (j != ctx.dumpedcount-1 ||
		       ctx.start + ctx.dumpedcount == ctx.totalcount ||
		       ctx.dumpedcount != ctx.maxcount_orig))
	idbW_NL();

      if (j == ctx.dumpedcount-1)
	break;
    }
  
  if (useTable)
    idbW_dest->flush("</th></tr></table>" DG_ECENTER "\n");
  else
    {
      dest->pop();
      dest->pop();
    }
}

static Status
idbWGetTagObjects(Collection *coll, idbWTextMode textmod,
		  idbWDumpCollContext &ctx)
{
  Iterator q(coll);
  if (q.getStatus()) {idbW_STPR(q.getStatus()); return q.getStatus();}

  ObjectArray obj_array;
  idbW_CHECK_S(q.scan(obj_array, ctx.maxcount, ctx.start));

  const char **tags = ctx.tags;
  Object **objs = ctx.objs;
  int *lens = ctx.lens;
  int offset = ctx.dumpedcount;

  int count = obj_array.getCount();

  for (int j = 0; j < count; j++)
    {
      Object *o = const_cast<Object *>(obj_array[j]);
      if (!o)
	{
	  tags[j+offset] = 0;
	  lens[j+offset] = 0;
	  continue;
	}

      idbW_ctx->dump_ctx.push(coll, idbW_member, j);

      idbW_CHECK_S(idbWGetTag(o, tags[j+offset],
			      textmod, lens[j+offset]));

      tags[j+offset] = strdup(tags[j+offset]);
      if (lens[j+offset] > ctx.maxlen)
	ctx.maxlen = lens[j+offset];

      if (idbW_ctx->exp_ctx.isExpanded(idbW_ctx->dump_ctx))
	{
	  objs[j+offset] = o;
	  obj_array.setObjectAt(j, 0); // obj_array[j] = 0;
	}
      else
	{
	  objs[j+offset] = 0;
	  o->release();
	  assert(!objs[j+offset]);
	}

      idbW_ctx->dump_ctx.pop();
    }

  ctx.add(count);
  //  obj_array.garbage();

  return Success;
}

Status
idbWDumpCollection(idbWDest *dest, Collection *coll,
		   const Class *cls, const char *item_name,
		   idbWTextMode textmod, idbWHeaderMode headmod,
		   Bool useTable, idbWDumpCollContext &ctx)
{

  Status s;

  if (coll)
    {
      if (idbW_ctx->verbmod != idbWVerboseFull && !coll->getCount())
	return Success;
      // scanning collection
      s = idbWGetTagObjects(coll, textmod, ctx);
      if (s) return s;
    }

  if (!ctx.isOk())
    return Success;

  // making prev context
  if (ctx.totaldumpedcount)
    {
      Oid prevoid;
      unsigned int prevstart = ctx.getPrevContext(prevoid);
      idbWMakeContinueContext(dest, "prev", cls, item_name,
			      prevoid, prevstart,
			      MAX(ctx.totaldumpedcount-ctx.collelemcount, 0),
			      textmod, True, useTable);
    }

  // making header
  Bool isExpanded = idbW_ctx->exp_ctx.isExpanded(idbW_ctx->dump_ctx);

  idbWDumpCollHead(dest, coll, cls, textmod, headmod, useTable,
		   isExpanded, ctx);
  
  // making body
  if (useTable || isExpanded)
    {
      idbWDumpCollBody(dest, coll, cls, textmod, headmod, useTable, ctx);

      if ((ctx.start + ctx.dumpedcount != ctx.totalcount) &&
	  ctx.dumpedcount == ctx.maxcount_orig)
	idbWMakeContinueContext(dest, "next", cls, item_name,
				coll->getOidC(), ctx.start+ctx.lastcount,
				ctx.totaldumpedcount + ctx.dumpedcount,
				textmod, True, useTable);
      else if (useTable)
	dest->flush("<br><br>");
    }
  
  return Success;
}

Status
idbWItemDumpCollection(idbWDest *dest, Object *o,
		       const Attribute *item,
		       idbWTextMode textmod, idbWVerboseMode verbmod,
		       idbWHeaderMode headmod)
{
  int dims = idbWGetSize(item, o);
  const Class *cls =
    ((CollectionClass *)item->getClass())->getCollClass();
  int done = 0;

  for (int n = 0; n < dims; n++)
    {
      Collection *set;

      idbW_CHECK_S(autoLoad(item, o, (Object **)&set, n));

      if (!set)
	continue;

      idbW_ctx->dump_ctx.push(o, item->getNum(), n);
      idbWDumpCollContext ctx(0, set, 0, item->getName(), False);
      idbW_CHECK_S(idbWDumpCollection(dest, set, cls, item->getName(),
				      textmod, idbWHeaderNo,
				      False, ctx));
      idbW_ctx->dump_ctx.pop();
      if (item->isIndirect())
	set->release();
    }

  if (done)
    idbW_dest->pop();

  return Success;
}

Status
idbWItemDumpEnum(idbWDest *dest, Object *o, const Attribute *item,
		 idbWTextMode textmod, idbWVerboseMode verbmod,
		 idbWHeaderMode headmod)
{
  int dims = idbWGetSize(item, o);
  Bool done = False;
  
  for (int i = 0; i < dims; i++)
    {
      int val;
      const char *enname = idbWGetEnumName(item, o, i, &val);
      headFlush(done, dest, item->getClass(), item->getName(), textmod, dims);
      dest->cflush(*enname ? enname : idbW_null);
      /*
      idbW_CHECK_FLUSH(idbW_dest->flush("%s: %s  \n",
			      idbWFieldName(item->getClass(),
					     item->getName(), textmod),
			      (*enname ? enname : idbW_null)));
			      */
    }

  if (done)
    idbW_NL();

  return Success;
}

#define HEAD_FLUSH(O) \
idbW_CHECK_FLUSH(idbW_dest->flush(make_ref())); \
if ((O)->getOid().isValid() && !idbWCheckMultiDB(0, (O)->getOid(), True)) \
  return Success; \
if (dims > 1 || item->isVarDim()) \
{ \
    idbW_CHECK_FLUSH(idbW_dest->cflush \
      ("%s[%d]:", idbWFieldName(fcls, name, textmod), n)); \
} \
else \
{ \
   idbW_CHECK_FLUSH(idbW_dest->cflush("%s:", \
	      idbWFieldName(fcls, name, textmod))); \
}

Status
idbWItemDumpObject(idbWDest *dest, Object *o, const Attribute *item,
		   idbWTextMode textmod, idbWVerboseMode verbmod,
		   idbWHeaderMode headmod)
{
  const Class *cls = item->getClass();
  const char *name = item->getName();

  const Class *fcls = item->getClass();

  int dims = idbWGetSize(item, o);
  int len;

  for (int n = 0; n < dims; n++)
    {
      Status status;
      Object *oo;
      
      idbW_CHECK_S(autoLoad(item, o, &oo, n));

      if (!oo)
	{
	  if (verbmod == idbWVerboseFull)
	    idbW_dest->flush("  %s: %s  \n", // added 2 spaces the 8/11/99
			     idbWFieldName(item->getClass(),
					   item->getName(), textmod),
			     idbW_null);
	  continue;
	}

      const char *item_name = "<empty>";

      if (item->getClass()->asAgregatClass())
	{
	  idbW_ctx->dump_ctx.push(o, item->getNum(), n);
	  if (idbW_ctx->exp_ctx.isExpanded(idbW_ctx->dump_ctx))
	    {
	      if (verbmod == idbWVerboseFull || !idbWIsEmpty(oo, item_name))
		{
		  HEAD_FLUSH(oo);
		  const char *tag;
		  idbW_CHECK_S(idbWGetTag(oo, tag, textmod, len));
		  dest->push();
		  if (strcmp(tag, idbW_skipTag))
		    idbW_dest->cflush(" %s  \n", tag);
		  idbW_CHECK_S(idbWDumpObject(dest, oo, textmod,
					      verbmod, idbWHeaderNo));
		  dest->pop();
		}
	    }
	  else
	    {
	      HEAD_FLUSH(oo);
	      const char *tag;
	      idbW_CHECK_S(idbWGetTag(oo, tag, textmod, len));
	      if (strcmp(tag, idbW_skipTag))
		idbW_dest->cflush(" %s  \n", tag);
	    }

	  idbW_ctx->dump_ctx.pop();
	  if (item->isIndirect())
	    oo->release();
	}
    }

  return Success;
}

int get_obj_count(ValueArray &val_array)
{
  int cnt = 0;
  for (int i = 0; i < val_array.getCount(); i++)
    if (val_array[i].type == Value::tOid)
      cnt++;

  return cnt;
}

static void
idbWPrintQueryResult(const char *s, int count, int objcount)
{
  idbW_dest->flush("<center><table border=%d><tr><th bgcolor=%s>",
		   idbW_db_config->display_config.border,
		   idbW_db_config->look_config.tablebgcolor);

  idbW_dest->flush("Result of the query '<i>%s</i>'<br>", idbWMakeHTMLTag(s));

  if (!count)
    idbW_dest->flush("No objects found");
  else if (count > objcount)
    {
      if (objcount)
	idbW_dest->flush("%d object%s + %d atoms found",
			 objcount, idbW_PLURAL(objcount),
			 count-objcount, idbW_PLURAL(count-objcount));
      else
	idbW_dest->flush("%d atom%s found",
			 count, idbW_PLURAL(count));
    }
  else
    idbW_dest->flush("%d object%s found",
		     count, idbW_PLURAL(count));

  idbW_dest->flush("</th></tr></table></center>\n\n");
}

int
idbWDumpGenRealize(idbWProcess *p, int fd)
{
  Oid *oid_array = 0;
  int oid_cnt = 0;
  int oid_alloc = 0;
  int center = 0;

  struct timeval tv;
  gettimeofday(&tv, NULL);
  idbW_CHECK(idbW_ctx->db->transactionBegin());
  gettimeofday(&tv, NULL);
  Bool oneObj = False;
  Status status;

  idbW_dest->flush("<pre><font2 size=-1>");
  for (int i = 0; i < idbW_ctx->etc_cnt; i++)
    {
      char *s = idbW_ctx->etc[i];
      Oid oid(s);

      if (!i && idbW_ctx->etc_cnt > 1)
	{
	  idbW_dest->push(); // ??
	  idbW_dest->flush("<center><table border=%d><tr><th bgcolor=%s>",
			   idbW_db_config->display_config.border,
			   idbW_db_config->look_config.tablebgcolor);
	  idbW_dest->flush("%d objects displayed", idbW_ctx->etc_cnt);
	  idbW_dest->flush("</th></tr></table></center>\n\n");
	}

      if (oid.isValid())
	{
	  oneObj = True;
	  Object *o;
	  status = idbW_ctx->db->loadObject(&oid, &o);
	  if (status)
	    {
	      idbW_STPR(status);
	      goto out;
	    }

	  if (false && idbW_ctx->etc_cnt == 1)
	    idbW_dest->flush("<center333><table border=%d>"
			     "<tr><th align=left bgcolor=%s><pre>",
			     idbW_db_config->display_config.border,
			     idbW_db_config->look_config.tablebgcolor);

	  if (idbWDumpGen(o))
	    {
	      if (false && idbW_ctx->etc_cnt == 1)
		idbW_dest->flush("</pre></th></tr></table></center333>\n");
	      o->release();
	      goto out;
	    }

	  idbWAddTo(oid_array, oid_cnt, oid_alloc, oid);
	  if (false && idbW_ctx->etc_cnt == 1)
	    idbW_dest->flush("</pre></th></tr></table></center333>\n");
	  idbW_dest->flush("\n\n");
	  o->release();
	}
      else
	{
	  OQL q(idbW_ctx->db, s);
	  
	  ValueArray val_array;
	  status = q.execute(val_array);

	  if (status)
	    {
	      std::string error = std::string("query: '") + s + "': " +
		status->getDesc();
	      IDB_LOG(IDB_LOG_WWW, ("error: '%s'\n", error.c_str()));
	      idbWPrintError(error.c_str());
	      return 1;
	    }

	  int count = val_array.getCount();
	  int objcount = get_obj_count(val_array);

	  idbWPrintQueryResult(s, count, objcount);

	  idbW_dest->push(); // ??
	  for (int j = 0; j < count; j++)
	    {
	      if (val_array[j].type == Value::tOid)
		{
		  Object *o;
		  status = idbW_ctx->db->loadObject(*val_array[j].oid, o);
		  if (status) { idbW_STPR(status); return 1; }

		  if (!o)
		    continue;

		  if (idbWDumpGen(o))
		    {
		      o->release();
		      goto out;
		    }

		  idbWAddTo(oid_array, oid_cnt, oid_alloc, o->getOid());
		  o->release();
		}
	      else
		idbW_dest->flush("%s\n", val_array[j].getString());
	      idbW_dest->cflush("\n");
	    }
	}
    }

  idbW_CHECK(idbW_ctx->db->transactionCommit());

  //idbW_dest->flush("<center>");
  //center = 1;

  if (idbW_nullCount || idbW_ctx->verbmod == idbWVerboseFull)
    idbWHrefRedisplay(oid_array, oid_cnt);

  free(oid_array);
  idbW_TABULAR();

out:
  if (!center)
    idbW_dest->flush("<center>");

  idbW_dest->flush("<br><br><center>");
  idbWBackTo();
  idbW_dest->flush("</center>");

  idbW_dest->flush("</pre>");
  return 0;
}

int
idbWDumpContextRealize(idbWProcess *p, int fd)
{
  int done = 0;

  idbW_CHECK(idbW_ctx->db->transactionBegin());

  Oid coll_oid(idbW_ctx->set_oid_str), mcl_oid(idbW_ctx->mcl_oid_str);
  Collection *coll;
  Class *cls;

  idbW_CHECK(idbW_ctx->db->loadObject(&coll_oid, (Object **)&coll));
  if (mcl_oid.isValid())
    idbW_CHECK(idbW_ctx->db->loadObject(&mcl_oid, (Object **)&cls));
  else
    cls = 0;

  Collection *ocoll = 0;
  if (!strcmp(idbW_ctx->item_name, idbWExtent))
    {
      idbWDumpCollContext ctx(cls, coll, atoi(idbW_ctx->start_str), 0);
      ocoll = coll;
      for (int n = 0; ;n++)
	{
	  idbW_ctx->dump_ctx.init(coll);
	  (void)idbWDumpCollection(idbW_dest, coll, cls, idbW_ctx->item_name,
				   idbW_ctx->glb_textmod, idbWHeaderYes,
				   True, ctx);
	  if (ctx.isOk())
	    break;

	  ctx.get_next_coll(coll);
	}
    }
  else
    {
      idbW_ctx->dump_ctx.init(coll);
      idbWDumpCollContext ctx(0, coll, atoi(idbW_ctx->start_str),
			      idbW_ctx->item_name);
      ocoll = coll;
      (void)idbWDumpCollection(idbW_dest, coll, cls, idbW_ctx->item_name,
			       idbW_ctx->glb_textmod, idbWHeaderYes, True,
			       ctx);
    }

  if (ocoll)
    ocoll->release();

  idbW_CHECK(idbW_ctx->db->transactionCommit());
  idbW_dest->cflush("\n\n<center>");
  idbWBackTo();
  idbW_dest->cflush("</center>");

  return 0;
}
