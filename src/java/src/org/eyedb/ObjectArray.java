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

public class ObjectArray {

  Object[] objects;

  public ObjectArray() {
    objects = null;
  }

  public ObjectArray(Object[] objects) {
    set(objects);
  }

  public ObjectArray(ObjectArray obj_array) {

    if (obj_array.objects == null) 
     {
	objects = null;
	return;
      }

    objects = new Object[obj_array.objects.length];

    for (int i = 0; i < objects.length; i++)
      objects[i] = obj_array.objects[i];
  }

  public ObjectArray(java.util.Vector v) {
    set(v);
  }

  public ObjectArray(java.util.LinkedList list) {
    set(list);
  }

  public void set(java.util.Vector v) {
   int n = 0;
   objects = new Object[v.size()];
   for (java.util.Enumeration e = v.elements(); e.hasMoreElements();)
      objects[n++] = (Object)e.nextElement();
  }

  public void set(java.util.LinkedList list) {
      objects = new Object[list.size()];
      java.util.ListIterator iter = list.listIterator(0);
      int n = 0;
      while (iter.hasNext())
	  objects[n++] = (Object)iter.next();
  }

  public int getCount() {return (objects == null ? 0 : objects.length);}

  public Object[] getObjects() {return objects;}

  public Object getObject(int idx) {return objects[idx];}

  public void append(Object o) {
    int olength = (objects == null ? 0 : objects.length);
    Object[] nobjects = new Object[olength + 1];

    for (int i = 0; i < olength; i++)
      nobjects[i] = objects[i];

    nobjects[olength] = o;
    objects = nobjects;
  }

  void set(Object[] objects) {
    this.objects = objects;
  }

  static ObjectArray[] copyArray(ObjectArray[] obj_array) {
    if (obj_array == null)
      return null;

    int len = obj_array.length;
    ObjectArray[] xobj_array = new ObjectArray[len];

    for (int i = 0; i < len; i++)
      if (obj_array[i] != null)
	xobj_array[i] = new ObjectArray(obj_array[i].objects);

    return xobj_array;
  }

  public java.util.Vector toVector() {
    java.util.Vector v = new java.util.Vector();
    if (objects != null)
	for (int i = 0; i < objects.length; i++)
	    v.add(objects[i]);
    return v;
  }
}

