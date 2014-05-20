#ifndef CYCLUS_SRC_BUILDING_MANAGER_H_
#define CYCLUS_SRC_BUILDING_MANAGER_H_

#include <map>
#include <vector>

#include "agent.h"
#include "builder.h"
#include "commodity_producer.h"
#include "OsiCbcSolverInterface.hpp"
#include "prog_translator.h"

namespace cyclus {
namespace toolkit {

/// a struct for a build order: the number of producers to build
struct BuildOrder {
  BuildOrder(int n, Builder* b,
             CommodityProducer* cp);
  int number;
  Builder* builder;
  CommodityProducer* producer;
};

/// The BuildingManager class is a managing entity that makes decisions
/// about which objects to build given certain conditions.
///
/// Specifically, the BuildingManager queries the SupplyDemandManager
/// to determine if there exists unmet demand for a Commodity, and then
/// decides which object(s) amongst that Commodity's producers should be
/// built to meet that demand. This decision takes the form of an
/// integer program:
///
/// \f[
/// \min \sum_{i=1}^{N}c_i*y_i \\
/// s.t. \sum_{i=1}^{N}\phi_i*y_i \ge \Phi \\
/// n_i \in [0,\infty) \forall i \in I, y_i integer
/// \f]
///
/// Where y_i is the number of objects of type i to build, c_i is the
/// cost to build the object of type i, \f$\phi_i\f$ is the nameplate
/// capacity of the object, and \f$\Phi\f$ is the capacity demand. Here
/// the set I corresponds to all producers of a given commodity.
class BuildingManager {
 public:
  BuildingManager(Agent* agent=NULL) : agent_(agent) {};

  /// register a builder with the manager
  /// @param builder the builder
  inline void Register(Builder* builder) {builders_.insert(builder);}

  /// unregister a builder with the manager
  /// @param builder the builder
  inline void Unregister(Builder* builder) {builders_.erase(builder);}

  /// given a certain commodity and demand, a decision is made as to
  /// how many producers of each available type to build this
  /// function constructs an integer program through the
  /// SolverInterface
  /// @param commodity the commodity being demanded
  /// @param demand the additional capacity required
  /// @return a vector of build orders as decided
  std::vector<BuildOrder> MakeBuildDecision(Commodity& commodity,
                                            double demand);

  inline const std::set<Builder*>& builders() const {
    return builders_;
  }

  inline Agent* agent() const {return agent_;}

 private:
  /// the agent managing this instance
  Agent* agent_;
  std::set<Builder*> builders_;

  void SetUp_(OsiCbcSolverInterface& iface,
              ProgTranslator::Context& ctx,
              std::map<CommodityProducer*, Builder*>& p_to_b,
              std::map<int, CommodityProducer*>& idx_to_p,
              Commodity& commodity,
              double demand);

  void Solve_(OsiCbcSolverInterface& iface,
              ProgTranslator::Context& ctx,
              std::map<CommodityProducer*, Builder*>& p_to_b,
              std::map<int, CommodityProducer*>& idx_to_p,
              std::vector<BuildOrder>& orders);
};
} // namespace toolkit
} // namespace cyclus
#endif  // CYCLUS_SRC_BUILDING_MANAGER_H_