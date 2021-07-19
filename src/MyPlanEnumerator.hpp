#include <mutable/mutable.hpp>


struct MyPlanEnumerator : m::PlanEnumerator
{
    void operator()(const m::QueryGraph &G, const m::CostFunction &CF, m::PlanTable &PT) const override;
};
