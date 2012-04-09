// BatchReactor.h
#if !defined(_BATCHREACTOR_H)
#define _BATCHREACTOR_H

#include "FacilityModel.h"

#include "Material.h"
#include "MatBuff.h"

#include "Logger.h"

#include <iostream>
#include <queue>

// Useful typedefs
typedef std::pair<std::string, mat_rsrc_ptr> Fuel; 
typedef std::pair<std::string, IsoVector> Recipe; 
typedef std::pair<Recipe, Recipe> FuelPair;

/**
   Defines all possible phases this facility can be in
 */
enum Phase {BEGIN, CONTINUE, REFUEL, END};

/**
   @class BatchReactor 
   This class is identical to the RecipeReactor, except that it
   operates in a batch-like manner, i.e. it refuels in batches.
 */
class BatchReactor : public FacilityModel  {
/* --------------------
 * all MODEL classes have these members
 * --------------------
 */
 public:  
  /**
     Constructor for the BatchReactor class. 
   */
  BatchReactor() {init();}
  
  /**
     Destructor for the BatchReactor class. 
   */
  virtual ~BatchReactor() {};

  /**
     Initialize all members during construction
   */
  void init();
    
  /**
     initialize an object from XML input 
   */
  virtual void init(xmlNodePtr cur);

  /**
     initialize an object by copying another 
   */
  virtual void copy(BatchReactor* src);

  /**
     This drills down the dependency tree to initialize all relevant 
     parameters/containers. 
      
     Note that this function must be defined only in the specific model 
     in question and not in any inherited models preceding it. 
      
     @param src the pointer to the original (initialized ?) model to be 
   */
  virtual void copyFreshModel(Model* src);

  /**
     Print information about this model 
   */
  virtual void print();

/* ------------------- */ 


/* --------------------
 * all COMMUNICATOR classes have these members
 * --------------------
 */
 public:
  /**
     When the facility receives a message, execute any transaction
   */
  virtual void receiveMessage(msg_ptr msg);

/* ------------------- */ 


/* --------------------
 * Facility MODEL class has these members
 * --------------------
 */
  /* /\** */
  /*    Transacted resources are extracted through this method */
      
  /*    @param order the msg/order for which resource(s) are to be prepared */
  /*    @return list of resources to be sent for this order */
  /*  *\/ */
  /* virtual std::vector<rsrc_ptr> removeResource(msg_ptr order); */

  /* /\** */
  /*    Transacted resources are received through this method */
      
  /*    @param msg the transaction to which these resource objects belong */
  /*    @param manifest is the set of resources being received */
  /*  *\/ */
  /* virtual void addResource(msg_ptr msg, */
  /*       		   std::vector<rsrc_ptr> manifest); */

  /* /\** */
  /*    The handleTick function specific to the BatchReactor. */
      
  /*    @param time the time of the tick */
  /*  *\/ */
  /* virtual void handleTick(int time); */

  /* /\** */
  /*    The handleTick function specific to the BatchReactor. */
      
  /*    @param time the time of the tock */
  /*  *\/ */
  /* virtual void handleTock(int time); */

/* ------------------- */ 


/* --------------------
 * _THIS_ MODEL class has these members
 * --------------------
 */
  /**
     set the cycle length 
   */
  void setCycleLength(int l) {cycle_length_ = l;}

  /**
     return the cycle length 
   */
  int cycleLength() {return cycle_length_;}

  /**
     set the core loading
   */
  void setCoreLoading(double size) {core_loading_ = size;}

  /**
     return the core loading
   */
  double coreLoading() {return core_loading_;}

  /**
     set the number of batches per core
   */
  void setNBatches(int n) {batches_per_core_ = n;}

  /**
     return the number of batches per core
   */
  int nBatches() {return batches_per_core_;}

  /**
     set the batch loading
   */
  void setBatchLoading(double size) {batch_loading_ = size;}

  /**
     return the batch loading
   */
  double batchLoading() {return batch_loading_;}

  /**
     set the facility lifetime 
   */
  void setLifetime(int l) {lifetime_ = l;}

  /**
     return the facility lifetime 
   */
  int lifetime() {return lifetime_;}

  /**
     set the input recipe 
   */
  void setInRecipe(IsoVector r) {in_recipe_ = r;}

  /**
     return the input recipe 
   */
  IsoVector inRecipe() {return in_recipe_;}

  /**
     set the output recipe 
   */
  void setOutRecipe(IsoVector r) {out_recipe_ = r;}

  /**
     set the cycle length 
   */
  IsoVector outRecipe() {return out_recipe_;}

  /**
     add a fuel pair 
      
     @param incommod the input commodity 
     @param inFuel the isotopics of the input fuel 
     @param outcommod the output commodity 
     @param outFuel the isotopics of the output fuel 
   */
  void addFuelPair(std::string incommod, IsoVector inFuel, 
		   std::string outcommod, IsoVector outFuel);

  /**
     return the input commodity 
   */
  std::string inCommod() {return fuelPairs_->front().first.first;}

  /**
     return the output commodity 
   */
  std::string outCommod() {return fuelPairs_->front().second.first;}

 private:
  /**
     The time between batch reloadings. 
   */
  int cycle_length_;

  /**
     The current month in the cycle
   */
  int month_in_cycle_;

  /**
     The number of months that a facility stays operational. 
   */
  int lifetime_;

  /**
     The number of batches per core
  */
  int batches_per_core_;
  
  /**
     The total mass per core
  */
  double core_loading_;

  /**
     The total mass per batch
  */
  double batch_loading_;

  /**
     The receipe of input materials. 
   */
  IsoVector in_recipe_;

  /**
     The receipe of the output material. 
   */
  IsoVector out_recipe_;

  /**
     The amout of input material still needed
     at a time step
   */
  double request_amount_;

  /**
     a matbuff for material before they enter the core
   */
  MatBuff* preCore_;

  /**
     a matbuff for material while they are inside the core
   */
  MatBuff* inCore_;

  /**
     a matbuff for material after they exit the core
   */
  MatBuff* postCore_;

  /**
     The BatchReactor has pairs of input and output fuel 
   */
  std::deque<FuelPair>* fuelPairs_;

  /**
     The list of orders to process on the Tock 
   */
  std::deque<msg_ptr>* ordersWaiting_;

  /**
     set the next phase
  */
  void setPhase(Phase p);

  /**
     send messages up through the institution 
      
     @param recipient the final recipient 
     @param trans the transaction to send 
   */
  void sendMessage(Communicator* recipient, Transaction trans);

  /**
     make reqests 
   */
  void makeRequests();

  /**
     make offers 
   */
  void makeOffers();
  
/* ------------------- */ 

};

#endif
