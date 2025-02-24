#ifndef APPS_SHARED_SEQUENCE_CONTEXT_H
#define APPS_SHARED_SEQUENCE_CONTEXT_H

#include <poincare/context_with_parent.h>
#include <poincare/expression.h>
#include <poincare/symbol.h>
#include "sequence_store.h"

namespace Shared {

class Sequence;

template<typename T>
class TemplatedSequenceContext {
public:
  TemplatedSequenceContext();
  T valueOfCommonRankSequenceAtPreviousRank(int sequenceIndex, int rank) const;
  void resetCache();
  bool iterateUntilRank(int n, SequenceStore * sequenceStore, SequenceContext * sqctx);

  int independentSequenceRank(int sequenceIndex) { return m_independentRanks[sequenceIndex]; }
  void setIndependentSequenceRank(int rank, int sequenceIndex) { m_independentRanks[sequenceIndex] = rank; }
  T independentSequenceValue(int sequenceIndex, int depth) { return m_independentRankValues[sequenceIndex][depth]; }
  void setIndependentSequenceValue(T value, int sequenceIndex, int depth) { m_independentRankValues[sequenceIndex][depth] = value; }
  void step(SequenceContext * sqctx, int sequenceIndex = -1);
private:
  constexpr static int k_maxRecurrentRank = 10000;
  /* Cache:
   * We use two types of cache :
   * The first one is used to to accelerate the
   * computation of values of recurrent sequences. We memoize the last computed
   * values of the sequences and their associated ranks (n and n+1 for instance).
   * Thereby, when another evaluation at a superior rank k > n+1 is called,
   * we avoid iterating from 0 but can start from n. This cache allows us to step
   * all of the sequences at once.
   *
   * The second one used used for fixed term computation. For instance, if a
   * sequence is defined using a fixed term of another, u(3) for instance, we
   * compute its value through the second type of cache. This way, we do not
   * erase the data stored in the first type of cache and we can compute the
   * values of each sequence at independent rank. This means that
   * (u(3), v(5), w(10)) can be computed at the same time.
   * This cache is therefore used for independent steps of sequences
   */
  int m_commonRank;
  T m_commonRankValues[SequenceStore::k_maxNumberOfSequences][SequenceStore::k_maxRecurrenceDepth+1];

  // Used for fixed computations
  int m_independentRanks[SequenceStore::k_maxNumberOfSequences];
  T m_independentRankValues[SequenceStore::k_maxNumberOfSequences][SequenceStore::k_maxRecurrenceDepth+1];
};

class SequenceContext : public Poincare::ContextWithParent {
public:
  SequenceContext(Poincare::Context * parentContext, SequenceStore * sequenceStore) :
    ContextWithParent(parentContext),
    m_sequenceStore(sequenceStore) {}
  /* u{n}, v{n} and w{n} must be parsed as sequences in the sequence app
   * so that u{n} can be defined as a function of v{n} without v{n} being
   * already defined.
   * So expressionTypForIdentifier returns Type::Sequence for u, v and w,
   * and calls the parent context in other cases.
   * The other methods (setExpressionForSymbolAbstract and
   * expressionForSymbolAbstract) always call the parent context. */
  Poincare::Context::SymbolAbstractType expressionTypeForIdentifier(const char * identifier, int length) override;
  template<typename T> T valueOfCommonRankSequenceAtPreviousRank(int sequenceIndex, int rank) {
    return static_cast<TemplatedSequenceContext<T>*>(helper<T>())->valueOfCommonRankSequenceAtPreviousRank(sequenceIndex, rank);
  }

  void resetCache() {
    m_floatSequenceContext.resetCache();
    m_doubleSequenceContext.resetCache();
  }

  template<typename T> bool iterateUntilRank(int n) {
    return static_cast<TemplatedSequenceContext<T>*>(helper<T>())->iterateUntilRank(n, m_sequenceStore, this);
  }

  template<typename T> int independentSequenceRank(int sequenceIndex) {
    return static_cast<TemplatedSequenceContext<T>*>(helper<T>())->independentSequenceRank(sequenceIndex);
  }

  template<typename T> void setIndependentSequenceRank(int rank, int sequenceIndex) {
    static_cast<TemplatedSequenceContext<T>*>(helper<T>())->setIndependentSequenceRank(rank, sequenceIndex);
  }

  template<typename T> T independentSequenceValue(int sequenceIndex, int depth) {
    return static_cast<TemplatedSequenceContext<T>*>(helper<T>())->independentSequenceValue(sequenceIndex, depth);
  }

  template<typename T> void setIndependentSequenceValue(T value, int sequenceIndex, int depth) {
    static_cast<TemplatedSequenceContext<T>*>(helper<T>())->setIndependentSequenceValue(value, sequenceIndex, depth);
  }

  template<typename T> void stepSequenceAtIndex(int sequenceIndex) {
    static_cast<TemplatedSequenceContext<T>*>(helper<T>())->step(this, sequenceIndex);
  }
  SequenceStore * sequenceStore() { return m_sequenceStore; }
  void tidyDownstreamPoolFrom(char * treePoolCursor) override;
private:
  TemplatedSequenceContext<float> m_floatSequenceContext;
  TemplatedSequenceContext<double> m_doubleSequenceContext;
  SequenceStore * m_sequenceStore;
  template<typename T> void * helper() { return sizeof(T) == sizeof(float) ? (void*) &m_floatSequenceContext : (void*) &m_doubleSequenceContext; }
};

}

#endif
