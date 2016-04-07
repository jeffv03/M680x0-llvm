//===- AArch64RegisterBankInfo -----------------------------------*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file declares the targeting of the RegisterBankInfo class for AArch64.
/// \todo This should be generated by TableGen.
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AARCH64_AARCH64REGISTERBANKINFO_H
#define LLVM_LIB_TARGET_AARCH64_AARCH64REGISTERBANKINFO_H

#include "llvm/CodeGen/GlobalISel/RegisterBankInfo.h"

namespace llvm {

class TargetRegisterInfo;

namespace AArch64 {
enum {
  GPRRegBankID = 0, /// General Purpose Registers: W, X.
  FPRRegBankID = 1, /// Floating Point/Vector Registers: B, H, S, D, Q.
  NumRegisterBanks
};
} // End AArch64 namespace.

/// This class provides the information for the target register banks.
class AArch64RegisterBankInfo : public RegisterBankInfo {
public:
  AArch64RegisterBankInfo(const TargetRegisterInfo &TRI);
  /// Get the cost of a copy from \p B to \p A, or put differently,
  /// get the cost of A = COPY B.
  unsigned copyCost(const RegisterBank &A,
                    const RegisterBank &B) const override;
};
} // End llvm namespace.
#endif