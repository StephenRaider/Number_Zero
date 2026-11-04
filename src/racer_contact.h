#pragma once
#include "vehicle.h"

namespace nz {
// Highway racer bodies retain the existing shared collision dimensions.
// Traffic uses its separate, unchanged forgiving contact rules.
constexpr double RacerContactLength=4.50,RacerContactWidth=1.96,RacerRoadEdge=6.05;
inline bool ResolveRacerContact(Vehicle &a,const CarSpec &aSpec,Vehicle &b,const CarSpec &bSpec,double dt=1./120.){
 const double dx=b.distance-a.distance,dy=b.lateral-a.lateral;
 const double overlapX=RacerContactLength-std::abs(dx),overlapY=RacerContactWidth-std::abs(dy);
 if(overlapX<=0||overlapY<=0)return false;
 const bool endContact=overlapX<overlapY;
 double normal=(endContact?dx:dy)>=0?1.:-1.;
 const double massA=aSpec.mass+75.,massB=bSpec.mass+75.;
 const double invA=1./massA,invB=1./massB,inverseMass=invA+invB;
 const double beforeNormalA=endContact?a.speed:a.lateralSpeed,beforeNormalB=endContact?b.speed:b.lateralSpeed;
 const double closing=(beforeNormalA-beforeNormalB)*normal;
 double positionInvA=invA,positionInvB=invB,groundCorrection=0;
 if(!endContact&&closing>0&&std::abs(std::abs(beforeNormalA)-std::abs(beforeNormalB))>1e-8){
  // Road-supported tyres can resist a gentle squeeze. They cannot absorb an
  // arbitrarily hard hit: support is bounded by grip, mass and elapsed time.
  // The same rule applies to either racer, never to the rival's identity.
  bool receiverB=std::abs(beforeNormalB)<=std::abs(beforeNormalA);
  const Vehicle &receiver=receiverB?b:a;const auto &spec=receiverB?bSpec:aSpec;
  double impulseDirection=receiverB?normal:-normal;
  if(receiver.steering*impulseDirection<=.05){
   double mass=receiverB?massB:massA,attackerMass=receiverB?massA:massB;
   // Mild static-contact firmness compensates for the assisted steering force.
   double support=1.20*SteeringResponsiveness*spec.grip*mass*9.81*std::max(0.,dt);
   double supportedFraction=std::clamp(support/(attackerMass*closing),0.,1.);
   (receiverB?positionInvB:positionInvA)*=1.-supportedFraction;
   double common=(massA*beforeNormalA+massB*beforeNormalB)/(massA+massB);
   groundCorrection=std::clamp((receiverB?beforeNormalB:beforeNormalA)-common,-support/(massA+massB),support/(massA+massB));
  }
 }
 const double positionInverseMass=positionInvA+positionInvB;
 // Separate along the shallowest overlap axis. A heavier body moves less.
 // At the road edge, pass blocked correction back to the other body instead
 // of leaving penetration or teleporting the pinned racer through the verge.
 // Keep numerical separation below a pixel; shrink it at smaller timesteps
 // so repeated micro run-ups do not change the resistance of a resting contact.
 const double resolution=std::clamp(dt*120.,.05,1.);
 const double separation=(endContact?overlapX:overlapY)+.002*resolution*resolution;
 if(endContact){a.distance-=normal*separation*positionInvA/positionInverseMass;b.distance+=normal*separation*positionInvB/positionInverseMass;}
 else{
  const double oldA=a.lateral,oldB=b.lateral;
  a.lateral=std::clamp(oldA-normal*separation*positionInvA/positionInverseMass,-RacerRoadEdge,RacerRoadEdge);
  b.lateral=std::clamp(oldB+normal*separation*positionInvB/positionInverseMass,-RacerRoadEdge,RacerRoadEdge);
  double missing=separation-((oldA-a.lateral)+(b.lateral-oldB))*normal;
  if(missing>1e-9){
   // The first body has spare room if the second hit its boundary, and vice versa.
   double move=std::min(missing,normal>0?a.lateral+RacerRoadEdge:RacerRoadEdge-a.lateral);
   a.lateral-=normal*move;missing-=move;
   b.lateral=std::clamp(b.lateral+normal*missing,-RacerRoadEdge,RacerRoadEdge);
  }
 }
 double &normalA=endContact?a.speed:a.lateralSpeed,&normalB=endContact?b.speed:b.lateralSpeed;
 if(closing>0){
  // Inelastic bump: share normal momentum, without bounce or a rival-only buff.
  const double impulse=closing/inverseMass;
  normalA-=normal*impulse*invA;normalB+=normal*impulse*invB;
  normalA+=groundCorrection;normalB+=groundCorrection;
  double &tangentA=endContact?a.lateralSpeed:a.speed,&tangentB=endContact?b.lateralSpeed:b.speed;
  // Small rubbing friction follows contact impulse and relative tangent motion.
  // Matched-speed rubbing never applies a repeated percentage speed penalty.
  const double tangentImpulse=std::clamp((tangentA-tangentB)/inverseMass,-.08*impulse,.08*impulse);
  tangentA-=tangentImpulse*invA;tangentB+=tangentImpulse*invB;
 }
 if(!endContact){
  if(b.lateral>=RacerRoadEdge-1e-6&&normal>0){b.lateralSpeed=std::min(0.,b.lateralSpeed);a.lateralSpeed=std::min(0.,a.lateralSpeed);}
  if(b.lateral<=-RacerRoadEdge+1e-6&&normal<0){b.lateralSpeed=std::max(0.,b.lateralSpeed);a.lateralSpeed=std::max(0.,a.lateralSpeed);}
  if(a.lateral<=-RacerRoadEdge+1e-6&&normal>0){a.lateralSpeed=std::max(0.,a.lateralSpeed);b.lateralSpeed=std::max(0.,b.lateralSpeed);}
  if(a.lateral>=RacerRoadEdge-1e-6&&normal<0){a.lateralSpeed=std::min(0.,a.lateralSpeed);b.lateralSpeed=std::min(0.,b.lateralSpeed);}
 }
 a.speed=std::max(0.,a.speed);b.speed=std::max(0.,b.speed);
 return true;
}
}
