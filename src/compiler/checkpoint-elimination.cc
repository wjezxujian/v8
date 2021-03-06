// Copyright 2016 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/compiler/checkpoint-elimination.h"

#include "src/compiler/node-properties.h"

namespace v8 {
namespace internal {
namespace compiler {

CheckpointElimination::CheckpointElimination(Editor* editor)
    : AdvancedReducer(editor) {}

namespace {

// The given checkpoint is redundant if it is effect-wise dominated by another
// checkpoint and there is no observable write in between. For now we consider
// a linear effect chain only instead of true effect-wise dominance.
bool IsRedundantCheckpoint(Node* node) {
  Node* effect = NodeProperties::GetEffectInput(node);
  while (effect->op()->HasProperty(Operator::kNoWrite) &&
         effect->op()->EffectInputCount() == 1) {
    if (effect->opcode() == IrOpcode::kCheckpoint) return true;
    effect = NodeProperties::GetEffectInput(effect);
  }
  return false;
}

}  // namespace

Reduction CheckpointElimination::ReduceCheckpoint(Node* node) {
  DCHECK_EQ(IrOpcode::kCheckpoint, node->opcode());
  if (IsRedundantCheckpoint(node)) {
    return Replace(NodeProperties::GetEffectInput(node));
  }
  return NoChange();
}

Reduction CheckpointElimination::ReduceReturn(Node* node) {
  DCHECK_EQ(IrOpcode::kReturn, node->opcode());
  Node* effect = NodeProperties::GetEffectInput(node);
  if (effect->opcode() == IrOpcode::kCheckpoint && effect->OwnedBy(node)) {
    // Any checkpoint that is wholly owned by a {Return} node can never be used
    // for an actual bailout and can hence be cut out of the effect chain.
    Node* replacement = NodeProperties::GetEffectInput(effect);
    NodeProperties::ReplaceEffectInput(node, replacement);
    return Changed(node);
  }
  return NoChange();
}

Reduction CheckpointElimination::Reduce(Node* node) {
  switch (node->opcode()) {
    case IrOpcode::kCheckpoint:
      return ReduceCheckpoint(node);
    case IrOpcode::kReturn:
      return ReduceReturn(node);
    default:
      break;
  }
  return NoChange();
}

}  // namespace compiler
}  // namespace internal
}  // namespace v8
