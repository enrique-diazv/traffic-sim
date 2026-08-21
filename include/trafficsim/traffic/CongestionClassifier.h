#ifndef TRAFFICSIM_TRAFFIC_CONGESTION_CLASSIFIER_H
#define TRAFFICSIM_TRAFFIC_CONGESTION_CLASSIFIER_H

#include "trafficsim/network/Road.h"
#include "trafficsim/traffic/RoadTrafficMetrics.h"

namespace trafficsim
{

class CongestionClassifier final
{
  public:
    [[nodiscard]] static RoadTrafficMetrics evaluate(const Road &road,
                                                     RoadTrafficObservation observation);
};

} // namespace trafficsim

#endif // TRAFFICSIM_TRAFFIC_CONGESTION_CLASSIFIER_H