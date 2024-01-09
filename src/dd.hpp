#ifndef __DD_HPP__
#define __DD_HPP__

#include <algorithm>
#include "node.hpp"
#include <vector>
#include <optional>
#include <set>
#include "hashtable.hpp"

class AbstractDD {
protected:
   Pool::Ptr _mem;
private:
   ANode::Ptr _root;
   ANode::Ptr _trg;
   std::set<int> _labels;
   std::vector<ANode::Ptr> _an;
   void addArc(Edge::Ptr e);
protected:
   virtual bool eq(ANode::Ptr f,ANode::Ptr s) const = 0;
   virtual bool neq(ANode::Ptr f,ANode::Ptr s) const = 0;
   virtual double better(double obj1,double obj2) const = 0;
   virtual double initialBest() const = 0;
   void computeBest();
public:
   typedef std::shared_ptr<AbstractDD> Ptr;
   AbstractDD(Pool::Ptr p,const std::set<int>& labels) : _mem(p),_labels(labels) {}
   virtual ~AbstractDD() {}
   virtual ANode::Ptr init() = 0;
   virtual ANode::Ptr target() = 0;
   virtual ANode::Ptr transition(ANode::Ptr src,int label) = 0;
   virtual ANode::Ptr merge(const ANode::Ptr first,const ANode::Ptr snd) = 0;
   virtual double cost(ANode::Ptr src,int label) = 0;
   void doIt();
   void print(std::ostream& os);
};
   
template <typename ST,
          class Compare = std::less<double>,
          typename IBL1 = ST(*)(),
          typename IBL2 = ST(*)(),
          typename STF  = std::optional<ST>(*)(const ST&,int),
          typename STC  = double(*)(const ST&,int),
          typename SMF  = std::optional<ST>(*)(const ST&,const ST&),
          class Equal = std::equal_to<ST>,
          class NotEqual=std::not_equal_to<ST>>
requires Printable<ST> && Hashable<ST>
class DD :public AbstractDD {
private:
   IBL1 _sti;
   IBL2 _stt;
   STF _stf;
   STC _stc;
   SMF _smf;
   Hashtable<ST,ANode::Ptr> _nmap;
   bool eq(ANode::Ptr f,ANode::Ptr s) const {
      auto fp = dynamic_cast<const Node<ST>*>(f.get());
      auto sp = dynamic_cast<const Node<ST>*>(s.get());
      return Equal{}(fp->get(),sp->get());
   }
   bool neq(ANode::Ptr f,ANode::Ptr s) const {
      auto fp = dynamic_cast<const Node<ST>*>(f.get());
      auto sp = dynamic_cast<const Node<ST>*>(s.get());
      return NotEqual{}(fp->get(),sp->get());
   }
   double better(double obj1,double obj2) const {
      return Compare{}(obj1,obj2) ? obj1 : obj2;
   }
   double initialBest() const {
      constexpr auto gr = std::is_same<Compare,std::greater<double>>::value;
      auto v = gr ? std::numeric_limits<double>::min() : std::numeric_limits<double>::max();
      return v;
   }
   ANode::Ptr makeNode(ST&& state) {
      ANode::Ptr at = nullptr;
      auto inMap = _nmap.get(state,at);
      if (inMap)
         return at;
      else {
         auto value = new (_mem) Node<ST>(_mem,std::move(state));
         _nmap.insert(value->get(),value);
         return value;
      }
   }
   ANode::Ptr init() {
      return makeNode(_sti());
   }
   ANode::Ptr target() {
      return makeNode(_stt());
   }
   ANode::Ptr transition(ANode::Ptr src,int label) {
      auto op = dynamic_cast<const Node<ST>*>(src.get());
      auto vs = _stf(op->get(),label);
      if (vs.has_value())
         return makeNode(std::move(vs.value()));
      else return nullptr;
   }
   double cost(ANode::Ptr src,int label) {
      auto op = dynamic_cast<const Node<ST>*>(src.get());
      auto cVal = _stc(op->get(),label);
      return cVal;
   }
   ANode::Ptr merge(const ANode::Ptr f,const ANode::Ptr s) {
      auto fp = dynamic_cast<const Node<ST>*>(f.get());
      auto sp = dynamic_cast<const Node<ST>*>(s.get());
      auto vs = _smf(fp->get(),sp->get());
      if (vs.has_value())
         return makeNode(std::move(vs.value()));
      else return nullptr;
   }
public:
   DD(Pool::Ptr p,IBL1 sti,IBL2 stt,STF stf,STC stc,SMF smf,const std::set<int>& labels)
      : AbstractDD(p,labels),
        _sti(sti),
        _stt(stt),
        _stf(stf),
        _stc(stc),
        _smf(smf),
        _nmap(p,128)
   {}
   template <class... Args>
   static AbstractDD::Ptr makeDD(Args&&... args) {
      return std::make_shared<DD>(std::forward<Args>(args)...);
   }
};

#endif
