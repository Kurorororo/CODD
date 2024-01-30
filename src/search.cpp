#include "search.hpp"
#include "node.hpp"
#include "heap.hpp"
#include <iostream>
#include <iomanip>

struct QNode {
   ANode::Ptr node;
   double    bound;
   bool operator()(const QNode& a,const QNode& b) {
      return a.bound < b.bound;
   }
   friend std::ostream& operator<<(std::ostream& os,const QNode& q) {
      return os << "QNode[" << *q.node << "," << q.bound << "]";
   }
};

void BAndB::search(Bounds& bnds)
{
   Pool mem;
   using namespace std;
   cout << "B&B searching..." << endl;
   bnds.attach(_theDD);
   AbstractDD::Ptr relaxed = _theDD->duplicate();
   AbstractDD::Ptr restricted = _theDD->duplicate();
   WidthBounded* ddr[2];
   _theDD->setStrategy(new Exact);
   relaxed->setStrategy(ddr[0] = new Relaxed(_mxw));
   restricted->setStrategy(ddr[1] = new Restricted(_mxw));
   Heap<QNode,QNode> pq(&mem,32);
   pq.insertHeap(QNode { _theDD->init(), _theDD->initialWorst() } );
   //Bounds bnds(_theDD);
   unsigned nNode = 0;
   while(!pq.empty()) {
      cout << "Extracting...." << pq.size() << endl;
      auto bbn = pq.extractMax();
#ifndef _NDEBUG     
      cout << "BOUNDS NOW: " << bnds << endl;
#endif      
      cout << "EXTRACTED:  " << *bbn.node << "\t(" << bbn.bound << ")" << " SZ:" << pq.size() << endl;
      if (!_theDD->isBetter(bbn.bound,bnds.getPrimal()))
          continue;
      nNode++;
      relaxed->reset();
      relaxed->makeInitFrom(bbn.node);
      relaxed->compute();
      cout << "RELAXED " << relaxed->currentOpt() << "\n";
      // to debug
      //relaxed->display();int w;cin >> w;      
      bool dualBetter = _theDD->isBetter(relaxed->currentOpt(),bnds.getPrimal());
      cout << "DUALBETTER " <<  (dualBetter? "Y" : "N") << "\n";
      if (dualBetter) {
         relaxed->update(bnds);
         restricted->reset();
         restricted->makeInitFrom(bbn.node);
         cout << "B-RESTRICTED..."  << endl;
         restricted->compute();
         cout << "A-RESTRICTED..."  << endl;
         bool primalBetter = _theDD->isBetter(restricted->currentOpt(),bnds.getPrimal());
         if (primalBetter)
            restricted->update(bnds);
         if (!restricted->isExact() && !relaxed->isExact()) {
            cout << "B-CUTSET... " << endl;
            auto cutSet =relaxed->computeCutSet(); 
            cout << "A-CUTSET... " << cutSet.size() << endl;
            for(auto n : cutSet) {
               //std::cout << "\tCS=" << *n << std::endl;
               auto nd = _theDD->duplicate(n); // we got a duplicate of the node.
               if (nd == bbn.node) { // the cutset is the root. Only way out: increase width.
                  for(auto i=0u;i < sizeof(ddr)/sizeof(WidthBounded*);i++)
                     ddr[i]->setWidth(ddr[i]->getWidth() + 1);
               }
               //pq.insertHeap(QNode {nd, relaxed->currentOpt()});
               cout << "B-INS-HEAP " << pq.size() << "\n";
               pq.insertHeap(QNode {nd, nd->getBound()+nd->getBackwardBound()});
               cout << "A-INS-HEAP " << pq.size() << "\n";
            }
         }
      }
   }
   cout << "Done: " << bnds << "\t #nodes:" <<  nNode << "\n";
}
/*
void BAndB::search()
{
   Pool mem;
   using namespace std;
   cout << "B&B searching..." << endl;
   AbstractDD::Ptr relaxed = _theDD->duplicate();
   AbstractDD::Ptr restricted = _theDD->duplicate();
   _theDD->setStrategy(new Exact);
   relaxed->setStrategy(new Relaxed(_mxw));
   restricted->setStrategy(new Restricted(_mxw));
   Heap<QNode,QNode> pq(&mem,32);
   pq.insertHeap(QNode { _theDD->init(), _theDD->initialWorst() } );
   Bounds bnds(_theDD);
   while(!pq.empty()) {
      auto bbn = pq.extractMax();
#ifndef _NDEBUG      
      cout << "BOUNDS NOW: " << bnds << endl;
      cout << "EXTRACTED:  " << *bbn.node << "\t(" << bbn.bound << ")" << endl;
#endif      
      restricted->reset();
      restricted->makeInitFrom(bbn.node);
      restricted->compute();
      bool primalBetter = _theDD->isBetter(restricted->currentOpt(),bnds.getPrimal());
      if (primalBetter)
         restricted->update(bnds);
      if (!restricted->isExact()) {
         relaxed->reset();
         relaxed->makeInitFrom(bbn.node);
         relaxed->compute();
         bool improving = _theDD->isBetter(relaxed->currentOpt(),bnds.getPrimal());
         if (improving) {
            relaxed->update(bnds);
            if (!relaxed->isExact())
               for(auto n : relaxed->computeCutSet()) {
                  auto nd = _theDD->duplicate(n); // we got a duplicate of the node.
                  pq.insertHeap(QNode {nd, relaxed->currentOpt()});
               }
         }         
      }
   }
   cout << "Done: " << bnds << "\n";
}
*/  
