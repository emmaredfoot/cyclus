// BatchReactor.cpp
// Implements the BatchReactor class
#include "BatchReactor.h"

#include "Logger.h"
#include "GenericResource.h"
#include "CycException.h"
#include "InputXML.h"
#include "Timer.h"

#include <iostream>
#include <queue>

using namespace std;

/**
  TICK
  TOCK
  RECIEVE MATERIAL
  SEND MATERIAL
 */

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void BatchReactor::init() { 
  preCore_ = new MatBuff();
  inCore_ = new MatBuff();
  postCore_ = new MatBuff();
  ordersWaiting_ = new deque<msg_ptr>();
  fuelPairs_ = new deque<FuelPair>();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void BatchReactor::init(xmlNodePtr cur) { 
  FacilityModel::init(cur);
  
  // move XML pointer to current model
  cur = XMLinput->get_xpath_element(cur,"model/BatchReactor");
  
  // initialize facility parameters
  setCycleLength( strtod( XMLinput->get_xpath_content(cur,"cyclelength"), NULL ) );
  setLifetime( strtol( XMLinput->get_xpath_content(cur,"lifetime"), NULL, 10 ) ); 
  setCoreLoading( strtod( XMLinput->get_xpath_content(cur,"coreloading"), NULL ) );
  setNBatches( strtol( XMLinput->get_xpath_content(cur,"batchespercore"), NULL, 10 ) ); 
  setBatchLoading( core_loading_ / batches_per_core_ );

  // all facilities require commodities - possibly many
  string recipe_name;
  string in_commod;
  string out_commod;
  xmlNodeSetPtr nodes = XMLinput->get_xpath_elements(cur, "fuelpair");

  // for each fuel pair, there is an in and an out commodity
  for (int i = 0; i < nodes->nodeNr; i++) {
    xmlNodePtr pair_node = nodes->nodeTab[i];

    // get commods
    in_commod = XMLinput->get_xpath_content(pair_node,"incommodity");
    out_commod = XMLinput->get_xpath_content(pair_node,"outcommodity");

    // get in_recipe
    recipe_name = XMLinput->get_xpath_content(pair_node,"inrecipe");
    setInRecipe(IsoVector::recipe(recipe_name));

    // get out_recipe
    recipe_name = XMLinput->get_xpath_content(pair_node,"outrecipe");
    setOutRecipe(IsoVector::recipe(recipe_name));

    fuelPairs_->push_back(make_pair(make_pair(in_commod,in_recipe_),
          make_pair(out_commod, out_recipe_)));
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void BatchReactor::copy(BatchReactor* src) {

  FacilityModel::copy(src);

  setCycleLength( src->cycleLength() ); 
  setLifetime( src->lifetime() );
  setCoreLoading( src->coreLoading() );
  setNBatches( src->nBatches() );
  setBatchLoading( coreLoading() / nBatches() ); 
  setInRecipe( src->inRecipe() );
  setOutRecipe( src->outRecipe() );
  fuelPairs_ = src->fuelPairs_;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void BatchReactor::copyFreshModel(Model* src) {
  copy(dynamic_cast<BatchReactor*>(src));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void BatchReactor::print() { 
  FacilityModel::print(); 
  LOG(LEV_DEBUG2, "RReact") << "    converts commodity {"
      << fuelPairs_->front().first.first
      << "} into commodity {"
      << this->fuelPairs_->front().second.first
      << "}.";
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void BatchReactor::receiveMessage(msg_ptr msg) {
  // is this a message from on high? 
  if(msg->supplier()==this){
    // file the order
    ordersWaiting_->push_front(msg);
    LOG(LEV_INFO5, "RReact") << name() << " just received an order.";
  }
  else {
    throw CycException("BatchReactor is not the supplier of this msg.");
  }
}

// //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
// vector<rsrc_ptr> BatchReactor::removeResource(msg_ptr msg) {
//   Transaction trans = msg->trans();

//   double newAmt = 0;

//   mat_rsrc_ptr m;
//   mat_rsrc_ptr newMat;
//   mat_rsrc_ptr toAbsorb;

//   // start with an empty manifest
//   vector<rsrc_ptr> toSend;

//   // pull materials off of the inventory stack until you get the trans amount
//   while (trans.resource->quantity() > newAmt && !inventory_.empty() ) {
//     for (deque<Fuel>::iterator iter = inventory_.begin(); 
//         iter != inventory_.end(); 
//         iter ++){
//       // be sure to get the right commodity
//       if (iter->first == trans.commod) {
//         m = iter->second;

//         // start with an empty material
//         newMat = mat_rsrc_ptr(new Material());

//         // if the inventory obj isn't larger than the remaining need, send it as is.
//         if (m->quantity() <= (trans.resource->quantity() - newAmt)) {
//           newAmt += m->quantity();
//           newMat->absorb(m);
//           inventory_.pop_front();
//         } else { 
//           // if the inventory obj is larger than the remaining need, split it.
//           toAbsorb = m->extract(trans.resource->quantity() - newAmt);
//           newAmt += toAbsorb->quantity();
//           newMat->absorb(toAbsorb);
//         }
//         toSend.push_back(newMat);
//         LOG(LEV_DEBUG2, "RReact") <<"BatchReactor "<< ID()
//           <<"  is sending a mat with mass: "<< newMat->quantity();
//       }
//     }
//   }    
//   return toSend;
// }

// //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
// void BatchReactor::addResource(msg_ptr msg, std::vector<rsrc_ptr> manifest) {
//   // grab each material object off of the manifest
//   // and move it into the stocks.
//   for (vector<rsrc_ptr>::iterator thisMat=manifest.begin();
//        thisMat != manifest.end();
//        thisMat++) {
//     LOG(LEV_DEBUG2, "RReact") <<"BatchReactor " << ID() << " is receiving material with mass "
//         << (*thisMat)->quantity();
//     stocks_.push_front(make_pair(msg->trans().commod, boost::dynamic_pointer_cast<Material>(*thisMat)));
//   }
// }

// //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
// void BatchReactor::handleTick(int time) {
//   LOG(LEV_INFO3, "RReact") << name() << " is ticking {";

//   // if at beginning of cycle, beginCycle()
//   // if stocks are empty, ask for a batch
//   // offer anything in the inventory
  
//   // BEGIN CYCLE
//   if(month_in_cycle_ == 1){
//     LOG(LEV_INFO4, "RReact") << " Beginning a new cycle";
//     this->beginCycle();
//   };

//   makeRequests();
//   makeOffers();
//   LOG(LEV_INFO3, "RReact") << "}";
// }

// //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
// void BatchReactor::makeRequests(){
//   LOG(LEV_INFO4, "RReact") << " making requests {";
//   // MAKE A REQUEST
//   if(this->stocksMass() == 0){
//     // It chooses the next in/out commodity pair in the preference lineup
//     Recipe request_commod_pair;
//     Recipe offer_commod_pair;
//     request_commod_pair = fuelPairs_.front().first;
//     offer_commod_pair = fuelPairs_.front().second;
//     string in_commod = request_commod_pair.first;
//     IsoVector in_recipe = request_commod_pair.second;

//     // It then moves that pair from the front to the back of the preference lineup
//     fuelPairs_.push_back(make_pair(request_commod_pair, offer_commod_pair));
//     fuelPairs_.pop_front();

//     // It can accept only a whole batch
//     double requestAmt;
//     double minAmt = in_recipe.mass();
//     // The Recipe Reactor should ask for a batch if there isn't one in stock.
//     double sto = this->stocksMass(); 
//     // subtract sto from batch size to get total empty space. 
//     // Hopefully the result is either 0 or the batch size 
//     double space = minAmt - sto; // KDHFLAG get minAmt from the input ?
//     // this will be a request for free stuff
//     double commod_price = 0;

//     if (space == 0) {
//       // don't request anything
//     } else if (space <= minAmt) {
//       MarketModel* market = MarketModel::marketForCommod(in_commod);
//       Communicator* recipient = dynamic_cast<Communicator*>(market);
//       // if empty space is less than monthly acceptance capacity
//       requestAmt = space;

//       // request a generic resource
//       gen_rsrc_ptr request_res = gen_rsrc_ptr(new GenericResource(in_commod, "kg", requestAmt));

//       // build the transaction and message
//       Transaction trans;
//       trans.commod = in_commod;
//       trans.minfrac = minAmt/requestAmt;
//       trans.is_offer = false;
//       trans.price = commod_price;
//       trans.resource = request_res;

//       LOG(LEV_INFO5, "RReact") << name() << " has requested " << request_res->quantity()
//                                << " kg of " << in_commod << ".";
//       sendMessage(recipient, trans);
//       // otherwise, the upper bound is the batch size
//       // minus the amount in stocks.
//     } else if (space >= minAmt) {
//       MarketModel* market = MarketModel::marketForCommod(in_commod);
//       Communicator* recipient = dynamic_cast<Communicator*>(market);
//       // if empty space is more than monthly acceptance capacity
//       requestAmt = capacity_ - sto;

//       // request a generic resource
//       gen_rsrc_ptr request_res = gen_rsrc_ptr(new GenericResource(in_commod, "kg", requestAmt));

//       // build the transaction and message
//       Transaction trans;
//       trans.commod = in_commod;
//       trans.minfrac = minAmt/requestAmt;
//       trans.is_offer = false;
//       trans.price = commod_price;
//       trans.resource = request_res;

//       LOG(LEV_INFO5, "RReact") << name() << " has requested " << request_res->quantity()
//                                << " kg of " << in_commod << ".";
//       sendMessage(recipient, trans);
//     }
//   }
//   LOG(LEV_INFO4, "RReact") << "}";
// }

// //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
// void BatchReactor::sendMessage(Communicator* recipient, Transaction trans){
//       msg_ptr msg(new Message(this, recipient, trans)); 
//       msg->setNextDest(facInst());
//       msg->sendOn();
// }

// //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
// void BatchReactor::makeOffers(){
//   LOG(LEV_INFO4, "RReact") << " making offers {";
//   // MAKE OFFERS
//   // decide how much to offer

//   // there is no minimum amount a null facility may send
//   double min_amt = 0;
//   // this will be an offer of free stuff
//   double commod_price = 0;

//   // there are potentially many types of batch in the inventory stack
//   double inv = this->inventoryMass();
//   // send an offer for each material on the stack 
//   string commod;
//   Communicator* recipient;
//   double offer_amt;
//   for (deque<pair<string, mat_rsrc_ptr > >::iterator iter = inventory_.begin(); 
//        iter != inventory_.end(); 
//        iter ++){
//     // get commod
//     commod = iter->first;
//     MarketModel* market = MarketModel::marketForCommod(commod);
//     // decide what market to offer to
//     recipient = dynamic_cast<Communicator*>(market);
//     // get amt
//     offer_amt = iter->second->quantity();

//     // make a material to offer
//     mat_rsrc_ptr offer_mat = mat_rsrc_ptr(new Material(out_recipe_));
//     offer_mat->print();
//     offer_mat->setQuantity(offer_amt);

//     // build the transaction and message
//     Transaction trans;
//     trans.commod = commod;
//     trans.minfrac = min_amt/offer_amt;
//     trans.is_offer = true;
//     trans.price = commod_price;
//     trans.resource = offer_mat;

//     LOG(LEV_INFO5, "RReact") << name() << " has offered " << offer_mat->quantity()
//                              << " kg of " << commod << ".";

//     sendMessage(recipient, trans);
//   }
//   LOG(LEV_INFO4, "RReact") << "}";
// }

// //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
// void BatchReactor::handleTock(int time) {
//   LOG(LEV_INFO3, "RReact") << name() << " is tocking {";
//   // at the end of the cycle
//   if (month_in_cycle_ > cycle_length_){
//     this->endCycle();
//   };

//   // check what orders are waiting, 
//   while(!ordersWaiting_.empty()){
//     msg_ptr order = ordersWaiting_.front();
//     order->approveTransfer();
//     ordersWaiting_.pop_front();
//   };
//   month_in_cycle_++;
//   LOG(LEV_INFO3, "RReact") << "}";
// }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void BatchReactor::addFuelPair(std::string incommod, IsoVector inFuel,
                                std::string outcommod, IsoVector outFuel) {
  fuelPairs_->push_back(make_pair(make_pair(incommod, inFuel),
                                 make_pair(outcommod, outFuel)));
}

// //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
// double BatchReactor::inventoryMass(){
//   double total = 0;

//   // Iterate through the inventory and sum the amount of whatever
//   // material unit is in each object.

//   for (deque< pair<string, mat_rsrc_ptr> >::iterator iter = inventory_.begin(); 
//        iter != inventory_.end(); 
//        iter ++){
//     total += iter->second->quantity();
//   }

//   return total;
// }

// //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
// double BatchReactor::stocksMass(){
//   double total = 0;

//   // Iterate through the stocks and sum the amount of whatever
//   // material unit is in each object.

//   if(!stocks_.empty()){
//     for (deque< pair<string, mat_rsrc_ptr> >::iterator iter = stocks_.begin(); 
//          iter != stocks_.end(); 
//          iter ++){
//         total += iter->second->quantity();
//     };
//   };
//   return total;
// }

// /* ------------------- */ 


/* --------------------
 * all MODEL classes have these members
 * --------------------
 */

extern "C" Model* constructBatchReactor() {
  return new BatchReactor();
}

/* ------------------- */ 

