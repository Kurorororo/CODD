/*
 * ddOpt is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License  v3
 * as published by the Free Software Foundation.
 *
 * ddOpt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY.
 * See the GNU Lesser General Public License  for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with mini-cp. If not, see http://www.gnu.org/licenses/lgpl-3.0.en.html
 *
 * Copyright (c)  2023. by Laurent Michel.
 *
 * Contributions by Waldemar Cruz, Rebecca Gentzel, Willem Jan Van Hoeve
 */

#ifndef __DDOPT_HASHTABLE_H
#define __DDOPT_HASHTABLE_H

#include <algorithm>
#include <iostream>
#include <functional>
#include <string>
#include <string.h>
#include "store.hpp"

template <class K,class T,class Hash = std::hash<K>,class Equal = std::equal_to<K>> class Hashtable {
   struct HTNode {
      K _key;
      T _data;
      HTNode* _next;
   };
   static constexpr const std::size_t _primes[] = {
      2,547,1229,1993,2749,3581,4421,5281,6143,7001,7927,8837,9739,10663,11677,12569,13513,14533,15413,16411,
      17393,18329,19427,20359,21391,22343,23327,24317,25409,26407,27457,28513,29453,30577,31607,32611,33617,        
      34651,35771,36787,37831,38923,39979,41113,42083,43063,44203,45317,46451,47533,48619,49667,50767,51817,
      52937,54001,55109,56197,57193,58243,59369,60509,61637,62791,63823,65071,66107,67247,68389,69497,70663,
      71719,72859,73999,75083,76213,77359,78487,79627,80737,81817,82903,84131,85243,86381,87557,88807,89867,
      90989,92177,93187,94351,95443,96587,97829,98953,100129,101287,102329,103613,104743,105953,107053,108271,
      109397,110603,111791,112997,114143,115327,116461,117673,118831,119993,121139,122263,123449,124601,125753,
      126989,128201,129287,130457,131707,132857,134059,135281,136393,137483,138647,139907,141067,142183,143503,
      144737,145933,147137,148361,149423,150589,151717,152897,154127,155333,156601,157747,159023,160183,161407,
      162671,163847,165083,166357,167483,168869,170047,171179,172399,173651,174799,176087,177211,178489,179581,
      180751,182089,183289,184477,185651,186761,187973,189253,190471,191677,192853,194017,195311,196561,197759,
      198943,200191,201511,202717,203971,205213,206383,207569,208739,209939
   };
   Pool::Ptr _pool;
   Hash _hash;
   HTNode**  _tab;
   unsigned* _mgc;
   std::size_t  _mxs;
   unsigned _magic;
   unsigned _nbp;   // number of pairs
public:
   Hashtable(Pool::Ptr p,std::size_t sz) : _pool(p) {
      constexpr const std::size_t tsz = sizeof(_primes)/sizeof(std::size_t);
      std::size_t low=0,up = tsz - 1;
      while (low <= up) {
         int m = (low + up)/2;
         if (sz < _primes[m])
            up = m - 1;
         else if (sz > _primes[m])
            low = m + 1;
         else {
            low = up = m;
            break;
         }
      }
      _mxs = low >= tsz ? _primes[tsz-1] :  _primes[low];
      std::cout << "sz(Hashtable):" << _mxs << '\n';
      _tab = new (_pool) HTNode*[_mxs];
      _mgc = new (_pool) unsigned[_mxs];
      memset(_tab,0,sizeof(HTNode*)*_mxs);
      memset(_mgc,0,sizeof(unsigned)*_mxs);
      _magic = 1;
      _nbp = 0;
   }
   void insert(const K& key,const T& val) noexcept {
      std::size_t at = _hash(key) % _mxs;
      HTNode* head = (_mgc[at]==_magic) ? _tab[at] : nullptr;
      HTNode* cur = head;
      while (cur != nullptr) {
         if (Equal{}(cur->_key,key)) {
            cur->_data = val;
            return ;
         }
         cur = cur->_next;
      }
      _tab[at] = new (_pool) HTNode {key,val,head};
      _mgc[at] = _magic;
      ++_nbp;
   }
   class HTAt {
      friend class Hashtable<K,T,Hash,Equal>;
      std::size_t _at;
      bool       _inc; // true if query is in the hashtable
      HTAt(std::size_t at,bool inc) : _at(at),_inc(inc) {}
   public:
      operator bool() const { return _inc;}
   };
   HTAt getLoc(const K& key,T& val) const noexcept {
      std::size_t at = _hash(key) % _mxs;
      assert(at >= 0);
      assert(at < _mxs);
      HTNode* cur =  (_mgc[at]==_magic) ? _tab[at] : nullptr;
      while (cur != nullptr) {
         if (Equal{}(cur->_key,key)) {
            val = cur->_data;
            return HTAt(at, true);
         }
         cur = cur->_next;
      }
      return HTAt(at, false);      
   }
   void safeInsertAt(const HTAt& loc,const K& key,const T& val) {
      assert(loc._inc == false);
      HTNode* head = (_mgc[loc._at]==_magic) ? _tab[loc._at] : nullptr;
      _tab[loc._at] = new (_pool) HTNode {key,val,head};
      _mgc[loc._at] = _magic;
      ++_nbp;      
   }
   void safeInsert(const K& key,const T& val) noexcept {
      std::size_t at = _hash(key) % _mxs;
      HTNode* head = (_mgc[at]==_magic) ? _tab[at] : nullptr;
      _tab[at] = new (_pool) HTNode {key,val,head};
      _mgc[at] = _magic;
      ++_nbp;
   }
   bool get(const K& key,T& val) const noexcept {
      std::size_t at = _hash(key) % _mxs;
      assert(at >= 0);
      assert(at < _mxs);
      HTNode* cur =  (_mgc[at]==_magic) ? _tab[at] : nullptr;
      while (cur != nullptr) {
         if (Equal{}(cur->_key,key)) {
            val = cur->_data;
            return true;
         }
         cur = cur->_next;
      }
      return false;      
   }
   unsigned size() const noexcept { return _nbp;}
   void clear() noexcept {
      ++_magic;
      //std::cout << "HT(" << this << ") had: " << _nbp << " entries\n";
      _nbp = 0;
   }
   template <class Fun>
   void doOnAll(Fun f) {
      for(auto i=0u;i < _mxs;i++) {
         HTNode* cur = (_mgc[i]==_magic) ? _tab[i] : nullptr;
         while (cur) {
            f(cur->_key,cur->_data);
            cur = cur->_next;
         }
      }
   }
};

#endif
