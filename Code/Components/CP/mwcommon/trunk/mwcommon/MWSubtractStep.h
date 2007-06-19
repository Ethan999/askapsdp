/// @file MWSubtractStep.h
/// @brief Step to process the MW subtract command.
///
/// @copyright (c) 2007 CONRAD, All Rights Reserved.
/// @author Ger van Diepen (diepen AT astron nl)
///
//# $Id$

#ifndef CONRAD_MWCOMMON_MWSUBTRACTSTEP_H
#define CONRAD_MWCOMMON_MWSUBTRACTSTEP_H

#include <mwcommon/MWStepBBS.h>
#include <vector>
#include <string>

namespace conrad { namespace cp {

  /// Step to process the MW subtract command.

  /// This class defines a step that subtracts a model from the data
  /// and writes the result into the VDS
  ///
  /// It uses the standard MWStep functionality (factory and visitor) to
  /// create and process the object.
  /// The object can be converted to/from blob, so it can be sent to workers.

  class MWSubtractStep: public MWStepBBS
  {
  public:
    MWSubtractStep();

    virtual ~MWSubtractStep();

    /// Clone the step object.
    virtual MWSubtractStep* clone() const;

    /// Create a new object of this type.
    static MWStep::ShPtr create();

    /// Register the create function in the MWStepFactory.
    static void registerCreate();

    /// Visit the object, so the visitor can process it.
    virtual void visit (MWStepVisitor&) const;

    /// Convert to/from blob.
    /// @{
    virtual void toBlob (LOFAR::BlobOStream&) const;
    virtual void fromBlob (LOFAR::BlobIStream&);
    /// @}
  };

}} /// end namespaces

#endif