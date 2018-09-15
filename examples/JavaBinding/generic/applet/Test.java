
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

import java.applet.Applet;
import java.awt.Image;
import java.awt.Event;
import java.awt.Graphics;
import java.awt.Dimension;
import java.io.StreamTokenizer;
import java.io.InputStream;
import java.io.BufferedInputStream;
import java.io.IOException;
import java.net.URL;
import java.util.Hashtable;
import java.awt.image.IndexColorModel;
import java.awt.image.ColorModel;
import java.awt.image.MemoryImageSource;

public class Test extends Applet implements Runnable {

  public void init() {
    org.eyedb.Root.init(this);

    try {
      org.eyedb.Connection conn = new org.eyedb.Connection();

      org.eyedb.Database db = new org.eyedb.Database(getParameter("database"));

      db.open(conn, org.eyedb.Database.DBRead);

      db.transactionBegin();

      org.eyedb.OQL q  = new org.eyedb.OQL(db, "select Person");

      org.eyedb.ValueArray value_array = new org.eyedb.ValueArray();

      q.execute(value_array);

      org.eyedb.Value[] values = value_array.getValues();

      for (int i = 0; i < value_array.getCount(); i++)
	if (values[i].getType() == org.eyedb.Value.OID)
	  {
	    org.eyedb.Object o = db.loadObject(values[i].getOid(),
					org.eyedb.RecMode.NoRecurs);
	    o.trace(System.out, org.eyedb.Object.ContentsTrace);
	  }

      conn.close();
    }

    catch(org.eyedb.Exception e) {
      e.print();
    }
  }

  public void run() {
  }

  public void start() {
  }

  public void stop() {
  }

  public void paint(Graphics g) {
    g.drawString("This is OK:", 3, 20);
  }

  public void update(Graphics g) {
    paint(g);
  }
}
