set(LLVM_TARGET_DEFINITIONS M680x0.td)

tablegen(LLVM M680x0GenRegisterInfo.inc     -gen-register-info)
tablegen(LLVM M680x0GenInstrInfo.inc        -gen-instr-info)
tablegen(LLVM M680x0GenSubtargetInfo.inc    -gen-subtarget)
tablegen(LLVM M680x0GenMCCodeBeads.inc      -gen-code-beads)
tablegen(LLVM M680x0GenMCPseudoLowering.inc -gen-pseudo-lowering)
tablegen(LLVM M680x0GenDAGISel.inc          -gen-dag-isel)
tablegen(LLVM M680x0GenCallingConv.inc      -gen-callingconv)
tablegen(LLVM M680x0GenAsmWriter.inc        -gen-asm-writer)

# M680x0CommonTableGen must be defined
add_public_tablegen_target(M680x0CommonTableGen)

# M680x0CodeGen should match with LLVMBuild.txt M680x0CodeGen
add_llvm_target(M680x0CodeGen
  M680x0AsmPrinter.cpp
  M680x0MCInstLower.cpp
  M680x0FrameLowering.cpp
  M680x0InstrInfo.cpp
  M680x0ISelLowering.cpp
  M680x0ISelDAGToDAG.cpp
  M680x0ExpandPseudo.cpp
  M680x0CollapseMOVEMPass.cpp
  M680x0MachineFunction.cpp
  M680x0RegisterInfo.cpp
  M680x0Subtarget.cpp
  M680x0TargetObjectFile.cpp
  M680x0TargetMachine.cpp
)

# Should match with "subdirectories =  MCTargetDesc TargetInfo" in LLVMBuild.txt
add_subdirectory(InstPrinter)
add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
