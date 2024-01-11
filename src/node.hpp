#ifndef __NODE_H
#define __NODE_H

#include <functional>
#include "vec.hpp"

template<typename T>
concept Printable = requires(std::ostream& os, const T& msg)
{
   {os << msg};
};

template<typename T>
concept Hashable = requires(const T& msg)
{
   {std::hash<T>{}(msg)};
};

template <Printable T> void print(std::ostream& os,const T& msg)
{
   os << msg;
}

class ANode;

struct Edge {
   typedef handle_ptr<Edge> Ptr;
   handle_ptr<ANode>  _from,_to;
   Vec<Edge::Ptr>::IdxType _fix,_tix;
   double _obj;
   int    _lbl;
   Edge(handle_ptr<ANode> fp,handle_ptr<ANode> tp,int lbl)
      : _from(fp),_to(tp),_fix(-1),_tix(-1),_obj(0),_lbl(lbl) {}
   friend std::ostream& operator<<(std::ostream& os,const Edge& e);
};

class ANode {
protected:
   Vec<Edge::Ptr,unsigned> _parents;
   Vec<Edge::Ptr,unsigned> _children;
   double                  _bound;
   unsigned                _layer; // will be used in restricted / (relaxed?)
   Vec<int,unsigned>       _optLabels;
   void addArc(Edge::Ptr ep);
public:
   friend class AbstractDD;
   typedef handle_ptr<ANode> Ptr;
   ANode(Pool::Ptr mem) : _parents(mem,2),_children(mem,2),_optLabels(mem) {}
   virtual ~ANode() {}
   virtual void print(std::ostream& os) const = 0;
   void setLayer(unsigned l) {  _layer = l;}
   auto getLayer() const   { return _layer;} 
   auto nbParents() const  { return _parents.size();}
   auto nbChildren() const { return _children.size();}
   auto beginPar()  { return _parents.begin();}
   auto endPar()    { return _parents.end();}
   auto beginKids() { return _children.begin();}
   auto endKids()   { return _children.end();}
   void setBound(double b) { _bound = b;}
   const auto getBound() const { return _bound;}
   void disconnect();
   friend std::ostream& operator<<(std::ostream& os,const ANode& s);
};


template <typename T> requires Printable<T> && Hashable<T>
class Node :public ANode {
   T _val;  
public:
   typedef handle_ptr<Node<T>> Ptr;
   Node(Pool::Ptr mem,T&& v) : ANode(mem),_val(std::move(v)) {}
   ~Node() {}
   const T& get() const {return _val;}
   void print(std::ostream& os) const {
      os << _val << ":B=" << _bound;
   }
};

#endif
