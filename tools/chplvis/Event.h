/*
 * Copyright 2015 Cray Inc.
 * Other additional copyright holders may be indicated within.
 *
 * The entirety of this work is licensed under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string>
#include <string.h>
#include <stdio.h>

#ifndef _EVENT_H_
#define _EVENT_H_

// Events generated by the runtime and VisualDebug.chpl

// Order important!  DataModel.cxx processes all events
//   all event types before and including Ev_end are grouped together
//   all event types after and including Ev_taks are inserted by time

// Order important, all before Ev_end are grouped together, after, inserted by time
enum Event_kind {Ev_start, Ev_tag, Ev_pause, Ev_end, Ev_task, Ev_comm, Ev_fork,
		 Ev_begin_task, Ev_end_task};

class Event {  // Base class for events 

  protected:
    long sec, usec;   //  Time of the event
    int  nodeid;      //  Which node generated the event

  public:
    Event(long esec, long eusec, int id)
      : sec(esec), usec(eusec), nodeid(id) {};

    long tsec () { return sec; }
    long tusec () { return usec; }
    double clock_time() { return sec+((double)usec/1000000); }

    virtual int Ekind() = 0;
    int nodeId() { return nodeid; }

    virtual void print() = 0;
};

// Comparison based on sec, usec 

bool operator < (Event &lh, Event &rh);
bool operator == (Event &lh, Event &rh);

class  E_start : public Event {

  private:
    long u_sec, u_usec;   // getrusage times
    long s_sec, s_usec;
     
  public:
    E_start (long esec, long eusec, int nid, long u_sec, long u_usec,
             long s_sec, long s_usec)
      : Event(esec, eusec, nid), u_sec(u_sec), u_usec(u_usec),
        s_sec(s_sec), s_usec(s_usec) {};

    double cpu_time()  { return u_sec+s_sec + ((double)u_usec+s_usec)/1000000; }
    double user_time() { return u_sec+(double)u_usec/1000000; }
    double sys_time() { return s_sec+(double)s_usec/1000000; }
    virtual int Ekind() {return Ev_start;}

    virtual void print() {
      printf ("Start: node %d time %ld.%06ld user %ld.%06ld sys %ld.%06ld\n",
              nodeid, sec, usec, u_sec, u_usec, s_sec, s_usec);
    }
};


class  E_task : public Event {

  private:
    long taskid;
    bool isOn;
    long lineNum;
    const char *srcFile;
  
  public:
    E_task (long esec, long eusec, int nid, long taskId, bool ison, long line,
            const char *file)
      : Event(esec,eusec, nid), taskid(taskId), isOn(ison), lineNum(line)
    {
      srcFile = file;  // Not duplicating, just saving the pointer
    }

    bool isLocal () { return !isOn; }
    long srcLine () { return lineNum; }
    const char *srcName () { return srcFile; }
    long taskId () { return taskid; }
    

    virtual int Ekind() {return Ev_task;}
    virtual void print() {
      printf ("Task: node %d time %ld.%06ld taskId %ld %s line %ld file %s\n",
              nodeid, sec, usec, taskid, isOn ? "OnExe" : "local", lineNum,
              (srcFile != NULL ? srcFile : "<none>"));
    }

};

class E_comm : public Event {

   private:
     int  dstid;
     int  elemsize, datalen;
     bool isget;
     long byTask;
     long lineNo;
     const char *srcFile;

   public:
     E_comm (long esec, long eusec, int esrcid, int edstid, int elSize,
             int dLen, bool get, long origTask, long line, const char *srcF) :
          Event(esec, eusec, esrcid), dstid(edstid), elemsize(elSize),
            datalen(dLen), isget(get), byTask(origTask), lineNo(line), srcFile(srcF) {};

     int srcId() { return nodeId(); }
     int dstId() { return dstid; }
     int elemSize() { return elemsize; }
     int dataLen() { return datalen; }
     int totalLen() { return elemsize * datalen; }
     bool isGet() { return isget; }
     long getLineNo() { return lineNo; }
     const char *srcName () { return srcFile; }
     long inTask() { return byTask; }

     virtual int Ekind() {return Ev_comm;}
     virtual void print() { 
       printf ("Comm: node %d time %ld.%06ld to %d size %d\n",
               nodeid, sec, usec, dstid, elemsize * datalen); }
};

class E_fork : public Event {

   private:
     int  dstid;
     int  argsize;
     bool isFast;
     long byTask;

   public:
       E_fork (long esec, long eusec, int esrcid, int edstid, int argsize,
               bool fast, long task) : Event(esec,eusec, esrcid), dstid(edstid),
       argsize(argsize), isFast(fast), byTask(task) {};

     int srcId() { return nodeId(); }
     int dstId() { return dstid; }
     bool fast() { return isFast; }
     int argSize() { return argsize; }
     long inTask() { return byTask; }

     virtual int Ekind() {return Ev_fork;}
     virtual void print() {
       printf ("Fork%s: node %d time %ld.%06ld to %d datasize %d\n",
               (isFast ? "(fast)" : ""), nodeid, sec, usec, dstid, argsize);
     }
};

class E_tag : public Event {

   private:
     int tag_num;
     const char *tag_name;
     long u_sec, u_usec;    // getrusage times
     long s_sec, s_usec;
     int vdbTask;

   public:
     E_tag (long esec, long eusec, int nodeid, long u_sec, long u_usec, long s_sec, long s_usec,
            long tagno, const char *tag, int tid)
       : Event(esec, eusec, nodeid), tag_num(tagno), tag_name(tag), u_sec(u_sec),
         u_usec(u_usec), s_sec(s_sec), s_usec(s_usec), vdbTask(tid)
       { }

     int tagNo() { return tag_num; }
     const char *tagName() { return tag_name; }

     double cpu_time()  { return u_sec+s_sec + ((double)u_usec+s_usec)/1000000; }
     double user_time() { return u_sec+(double)u_usec/1000000; }
     double sys_time() { return s_sec+(double)s_usec/1000000; }
     int vdbTid() { return vdbTask; }

     virtual int Ekind() {return Ev_tag;}
     virtual void print() {
       printf ("Tag: node %d time %ld.%06ld user %ld.%06ld sys %ld.%06ld tagNo %d, Tag='%s'\n",
               nodeid, sec, usec, u_sec, u_usec, s_sec, s_usec, tag_num, tag_name);
     }

};

class E_pause : public Event {

  private:
    long u_sec, u_usec;   // getrusage times
    long s_sec, s_usec;
    int tagid;
    int vdbTask;

  public:
    E_pause (long esec, long eusec, int nodeid, long u_sec, long u_usec,
             long s_sec, long s_usec, int tagid, int tid)
      : Event(esec, eusec, nodeid), u_sec(u_sec), u_usec(u_usec),
        s_sec(s_sec), s_usec(s_usec), tagid(tagid), vdbTask(tid) {};

    double cpu_time()  { return u_sec+s_sec + ((double)u_usec+s_usec)/1000000; }
    double user_time() { return u_sec+(double)u_usec/1000000; }
    double sys_time() { return s_sec+(double)s_usec/1000000; }
    int tagId() { return tagid; }
    int vdbTid() { return vdbTask; }
    virtual int Ekind() { return Ev_pause; }
    virtual void print() {
      printf ("Pause:  id %d time %ld.%06ld user %ld.%06ld sys %ld.%06ld tagNo %d\n",
              nodeid, sec, usec, u_sec, u_usec, s_sec, s_usec, tagid); 
    }
};

class E_end : public Event {

  private:
    long u_sec, u_usec;   // getrusage times
    long s_sec, s_usec;
    int vdbTask;

  public:
    E_end (long esec, long eusec, int nodeid, long u_sec, long u_usec, 
           long s_sec, long s_usec, int tid)
      : Event(esec, eusec, nodeid), u_sec(u_sec), u_usec(u_usec),
        s_sec(s_sec), s_usec(s_usec), vdbTask(tid) {};

    double cpu_time()  { return u_sec+s_sec + ((double)u_usec+s_usec)/1000000; }
    double user_time() { return u_sec+(double)u_usec/1000000; }
    double sys_time() { return s_sec+(double)s_usec/1000000; }
    int vdbTid() { return vdbTask;}
    virtual int Ekind() { return Ev_end; }
    virtual void print() {
      printf ("End: node %d time %ld.%06ld user %ld.%06ld sys %ld.%06ld\n",
              nodeid, sec, usec, u_sec, u_usec, s_sec, s_usec);
    }
};

class E_begin_task : public Event {

  private:
    long taskid;

  public:
    E_begin_task (long esec, long eusec, int nodeid, long tid)
      : Event(esec, eusec, nodeid), taskid(tid) {};

    long taskId() { return taskid; }
    virtual int Ekind() { return Ev_begin_task; }
    virtual void print() {
      printf ("Btask: node %d time %ld.%06ld taskId %ld\n",
              nodeid, sec, usec, taskid);
    }
};

class E_end_task : public Event {

   private:
     long taskid;

   public:
     E_end_task (long esec, long eusec, int nodeid, long tid)
       : Event(esec, eusec, nodeid), taskid(tid) {};

     long taskId() { return taskid; }
     virtual int Ekind() { return Ev_end_task; }
     virtual void print() {
       printf ("Etask: node %d time %ld.%06ld taskId %ld\n",
               nodeid, sec, usec, taskid);
     }
};

#endif
