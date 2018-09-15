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

public class TransactionParams {

  /* lock modes */
  static public final int DefaultLock = 0;
  static public final int LockN = 1;
  static public final int LockX = 2;
  static public final int LockSX = 3;
  static public final int LockS = 4;
  static public final int LockP = 5;
  static public final int LockE = 6;

  /* transaction mode */
  static public final int TransactionOff = 1 /* no rollback is possible */;
  static public final int TransactionOn = 2;

  /* transaction lock mode */
  static public final int ReadSWriteS = 1 /* read shared; write shared */;
  static public final int ReadSWriteSX = 1 /* read shared; write shared */+1;
  static public final int ReadSWriteX = 1 /* read shared; write shared */+2;
  static public final int ReadSXWriteSX = 1 /* read shared; write shared */+3;
  static public final int ReadSXWriteX = 1 /* read shared; write shared */+4;
  static public final int ReadXWriteX = 1 /* read shared; write shared */+5;
  static public final int ReadNWriteS = 1 /* read shared; write shared */+6;
  static public final int ReadNWriteSX = 1 /* read shared; write shared */+7;
  static public final int ReadNWriteX = 1 /* read shared; write shared */+8;
  static public final int ReadNWriteN = 1 /* read shared; write shared */+9;
  static public final int DatabaseX = 1 /* read shared; write shared */+10;

  /* transaction recovery mode */
  static public final int RecoveryOff = 3 /* prevents from remote client failure */;
  static public final int RecoveryPartial = 3 /* prevents from remote client failure */+1;
  static public final int RecoveryFull = 3 /* prevents from remote client failure */+2;

  public TransactionParams() {
  }

  public TransactionParams(int trsmode) {
    this.trsmode = trsmode;
  }

  public TransactionParams(int trsmode, int lockmode) {
    this.trsmode = trsmode;
    this.lockmode = lockmode;
  }

  public TransactionParams(int trsmode, int lockmode, int recovmode) {
    this.trsmode = trsmode;
    this.lockmode = lockmode;
    this.recovmode = recovmode;
  }

  public TransactionParams(int trsmode, int lockmode, int recovmode,
			      int magorder) {
    this.trsmode = trsmode;
    this.lockmode = lockmode;
    this.recovmode = recovmode;
    this.magorder = magorder;
  }

  public TransactionParams(int trsmode, int lockmode, int recovmode,
			      int magorder, int ratioalrt) {
    this.trsmode = trsmode;
    this.lockmode = lockmode;
    this.recovmode = recovmode;
    this.magorder = magorder;
    this.ratioalrt = ratioalrt;
  }

  public TransactionParams(int trsmode, int lockmode, int recovmode,
			      int magorder, int ratioalrt, int wait_timeout) {
    this.trsmode = trsmode;
    this.lockmode = lockmode;
    this.recovmode = recovmode;
    this.magorder = magorder;
    this.ratioalrt = ratioalrt;
    this.wait_timeout = wait_timeout;
  }

  public static TransactionParams getDefaultParams() {
    if (default_params == null)
      default_params = new TransactionParams();
    return default_params;
  }

  public int getTransactionMode() {
    return trsmode;
  }

  public int getLockMode() {
    return lockmode;
  }

  public int getRecoveryMode() {
    return recovmode;
  }

  public int getMagnitudeOrder() {
    return magorder;
  }

  public int getRatioAlert() {
    return ratioalrt;
  }

  public int getWaitTimeout() {
    return wait_timeout;
  }

  public static String getTransactionModeString(int trsmode) {
    if (trsmode == TransactionOn)
      return "TransactionOn";
    if (trsmode == TransactionOff)
      return "TransactionOff";
    return "<UNKNOWN TRANSACTION MODE>";
  }

  public static String getLockModeString(int lockmode) {
    if (lockmode == ReadSWriteS)
      return "ReadSWriteS";
    if (lockmode == ReadSWriteSX)
      return "ReadSWriteSX";
    if (lockmode == ReadSWriteX)
      return "ReadSWriteX";
    if (lockmode == ReadSXWriteSX)
      return "ReadSXWriteSX";
    if (lockmode == ReadSXWriteX)
      return "ReadSXWriteX";
    if (lockmode == ReadXWriteX)
      return "ReadXWriteX";
    if (lockmode == ReadNWriteS)
      return "ReadNWriteS";
    if (lockmode == ReadNWriteSX)
      return "ReadNWriteSX";
    if (lockmode == ReadNWriteX)
      return "ReadNWriteX";
    if (lockmode == ReadNWriteN)
      return "ReadNWriteN";
    if (lockmode == DatabaseX)
      return "DatabaseX";
    return "<UNKNOWN LOCK MODE>";
  }

  public static String getRecoveryModeString(int recovmode) {
    if (recovmode == RecoveryOff)
      return "RecoveryOff";
    if (recovmode == RecoveryPartial)
      return "RecoveryPartial";
    if (recovmode == RecoveryFull)
      return "RecoveryFull";
    return "<UNKNOWN RECOVERY MODE>";
  }
    
  public void trace(java.io.PrintStream out) {
    out.println("Transaction mode: " + getTransactionModeString(trsmode));
    out.println("Lock mode: " + getLockModeString(lockmode));
    out.println("Recovery mode: " + getRecoveryModeString(recovmode));
    out.println("Magnitude order: " + magorder);
    out.println("Ratio alert: " + ratioalrt);
    out.println("Wait timeout: " + wait_timeout);
  }

  private int trsmode = TransactionOn;
  private int lockmode = ReadNWriteSX;
  private int recovmode = RecoveryFull;
  private int magorder = 0; /* estimated object cardinality in transaction */
  private int ratioalrt = 0; /* error returned if ratio != 0 &&
				trans object number > ratioalrt * magorder  */
  private int wait_timeout = 30; /* in seconds */
  private static TransactionParams default_params;
}
