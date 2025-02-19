#include "equation_store.h"
#include <apps/constant.h>
#include <apps/shared/expression_display_permissions.h>
#include <apps/shared/poincare_helpers.h>
#include <apps/global_preferences.h>
#include <limits.h>
#include <poincare/addition.h>
#include <poincare/constant.h>
#include <poincare/division.h>
#include <poincare/opposite.h>
#include <poincare/matrix.h>
#include <poincare/multiplication.h>
#include <poincare/polynomial.h>
#include <poincare/power.h>
#include <poincare/rational.h>
#include <poincare/square_root.h>
#include <poincare/subtraction.h>
#include <poincare/symbol.h>
#include <poincare/undefined.h>

using namespace Poincare;
using namespace Shared;

namespace Solver {

EquationStore::EquationStore() :
  ExpressionModelStore(),
  m_degree(-1),
  m_numberOfSolutions(0),
  m_numberOfUserVariables(0),
  m_type(Type::LinearSystem)
{
}

Ion::Storage::Record::ErrorStatus EquationStore::addEmptyModel() {
  char name[3];
  static_assert(k_maxNumberOfEquations < 9, "Equation name record might not fit");
  const char * const extensions[1] = { Ion::Storage::eqExtension };
  name[0] = 'e';
  Ion::Storage::FileSystem::sharedFileSystem()->firstAvailableNameFromPrefix(name, 1, 3, extensions, 1, k_maxNumberOfEquations);
  return Ion::Storage::FileSystem::sharedFileSystem()->createRecordWithExtension(name, Ion::Storage::eqExtension, nullptr, 0);
}

Shared::ExpressionModelHandle * EquationStore::setMemoizedModelAtIndex(int cacheIndex, Ion::Storage::Record record) const {
  assert(cacheIndex >= 0 && cacheIndex < maxNumberOfMemoizedModels());
  m_equations[cacheIndex] = Equation(record);
  return &m_equations[cacheIndex];
}

ExpressionModelHandle * EquationStore::memoizedModelAtIndex(int cacheIndex) const {
  assert(cacheIndex >= 0 && cacheIndex < maxNumberOfMemoizedModels());
  return &m_equations[cacheIndex];
}

void EquationStore::tidyDownstreamPoolFrom(char * treePoolCursor) {
  ExpressionModelStore::tidyDownstreamPoolFrom(treePoolCursor);
  tidySolution(treePoolCursor);
}

Poincare::Layout EquationStore::exactSolutionLayoutAtIndex(int i, bool exactLayout) {
  assert(m_type != Type::Monovariable && i >= 0 && (i < m_numberOfSolutions));
  if (exactLayout) {
    return m_exactSolutionExactLayouts[i];
  } else {
    return m_exactSolutionApproximateLayouts[i];
  }
}

double EquationStore::intervalBound(int index) const {
  assert(m_type == Type::Monovariable && index >= 0 && index < 2);
  return m_intervalApproximateSolutions[index];
}

void EquationStore::setIntervalBound(int index, double value) {
  assert(m_type == Type::Monovariable && index >= 0 && index < 2);
  m_intervalApproximateSolutions[index] = value;
  if (m_intervalApproximateSolutions[0] > m_intervalApproximateSolutions[1]) {
    if (index == 0) {
      m_intervalApproximateSolutions[1] = m_intervalApproximateSolutions[0]+1;
    } else {
      m_intervalApproximateSolutions[0] = m_intervalApproximateSolutions[1]-1;
    }
  }
}

void EquationStore::approximateSolve(Poincare::Context * context, bool shouldReplaceFunctionsButNotSymbols) {
  m_hasMoreThanMaxNumberOfApproximateSolution = false;
  Expression undevelopedExpression = modelForRecord(definedRecordAtIndex(0))->standardForm(context, shouldReplaceFunctionsButNotSymbols, ReductionTarget::SystemForApproximation);
  m_userVariablesUsed = !shouldReplaceFunctionsButNotSymbols;
  assert(m_variables[0][0] != 0 && m_variables[1][0] == 0);
  assert(m_type == Type::Monovariable);
  m_numberOfSolutions = 0;

  double start = m_intervalApproximateSolutions[0];
  double end = m_intervalApproximateSolutions[1];
  assert(start <= end);
  Poincare::Solver<double> solver = PoincareHelpers::Solver(start, end, m_variables[0], context);
  solver.stretch();

  for (int i = 0; i <= k_maxNumberOfApproximateSolutions; i++) {
    double root = solver.nextRoot(undevelopedExpression).x1();
    if (root < start) {
      i--;
      continue;
    } else if (root > end) {
      root = NAN;
    }
    if (i == k_maxNumberOfApproximateSolutions) {
      m_hasMoreThanMaxNumberOfApproximateSolution = !std::isnan(root);
    } else {
      m_approximateSolutions[i] = root;
      if (std::isnan(root)) {
        break;
      }
      m_numberOfSolutions++;
    }
  }
}

EquationStore::Error EquationStore::exactSolve(Poincare::Context * context, bool * replaceFunctionsButNotSymbols) {
  assert(replaceFunctionsButNotSymbols != nullptr);
  // First, solve the equation using predefined variables if there are
  *replaceFunctionsButNotSymbols = false;

  Error firstError = privateExactSolve(context, false);
  int firstNumberOfSolutions = numberOfSolutions();

  if (m_numberOfUserVariables > 0 && (firstError != Error::NoError || firstNumberOfSolutions == 0 || firstNumberOfSolutions == INT_MAX)) {
    // If there were predefined variables, and solution isn't good, try without
    *replaceFunctionsButNotSymbols = true;
    Error secondError = privateExactSolve(context, true);

    if (firstError == Error::NoError && secondError != Error::NoError && secondError != Error::RequireApproximateSolution) {
      // First solution was better, but has been tidied. It is recomputed.
      *replaceFunctionsButNotSymbols = false;
      return privateExactSolve(context, false);
    }
    return secondError;
  }
  return firstError;
}

/* Equations are solved according to the following procedure :
 * 1) We develop the equations using the reduction target "SystemForAnalysis".
 * This expands structures like Newton multinoms and allows us to detect
 * polynoms afterwards. ("(x+2)^2" in this form is not detected but is if
 * expanded).
 * 2) We look for classic forms of equations for which we have algorithms
 * that output the exact answer. If one is recognized in the input equation,
 * the exact answer is given to the user.
 * 3) If no classic form has been found in the developped form, we need to use
 * numerical approximation. Therefore, to prevent precision losses, we work
 * with the undeveloped form of the equation. Therefore we set reductionTarget
 * to SystemForApproximation. Solutions are then numerically approximated
 * between the bounds provided by the user. */

EquationStore::Error EquationStore::privateExactSolve(Poincare::Context * context, bool replaceFunctionsButNotSymbols) {
  m_userVariablesUsed = !replaceFunctionsButNotSymbols;
  SymbolicComputation symbolicComputation = replaceFunctionsButNotSymbols ? SymbolicComputation::ReplaceDefinedFunctionsWithDefinitions : SymbolicComputation::ReplaceAllDefinedSymbolsWithDefinition;

  // Step 1. Get unknown and user-defined variables
  m_variables[0][0] = 0;
  int numberOfVariables = 0;

  // TODO we look twice for variables but not the same, is there a way to not do the same work twice?
  m_userVariables[0][0] = 0;
  m_numberOfUserVariables = 0;
  Expression simplifiedExpressions[k_maxNumberOfEquations];

  int definedModelsTotal = numberOfDefinedModels();
  for (int i = 0; i < definedModelsTotal; i++) {
    Shared::ExpiringPointer<Equation> eq = modelForRecord(definedRecordAtIndex(i));

    /* Start by looking for user variables, so that if we escape afterwards, we
     * know  if it might be due to a user variable. */
    if (m_numberOfUserVariables < Expression::k_maxNumberOfVariables) {
      const Expression eWithSymbols = eq->standardForm(context, true, ReductionTarget::SystemForAnalysis);
      /* If replaceFunctionsButNotSymbols is true we can memoize the expressions
       * for the rest of the function. Otherwise, we will memoize them at the
       * next call to standardForm */
      if (replaceFunctionsButNotSymbols == true) {
        simplifiedExpressions[i] = eWithSymbols;
      }
      int varCount = eWithSymbols.getVariables(context, [](const char * symbol, Poincare::Context * context) { return context->expressionTypeForIdentifier(symbol, strlen(symbol)) == Poincare::Context::SymbolAbstractType::Symbol; }, (char *)m_userVariables, Poincare::SymbolAbstract::k_maxNameSize, m_numberOfUserVariables);
      m_numberOfUserVariables = varCount < 0 ? Expression::k_maxNumberOfVariables : varCount;
    }
    if (simplifiedExpressions[i].isUninitialized()) {
      // The expression was not memoized before.
      simplifiedExpressions[i] = eq->standardForm(context, replaceFunctionsButNotSymbols, ReductionTarget::SystemForAnalysis);
    }
    const Expression e = simplifiedExpressions[i];
    if (e.isUninitialized() || e.type() == ExpressionNode::Type::Undefined || e.recursivelyMatches(Expression::IsMatrix, context, symbolicComputation)) {
      return Error::EquationUndefined;
    }
    if (e.type() == ExpressionNode::Type::Nonreal) {
      return Error::EquationNonreal;
    }
    numberOfVariables = e.getVariables(context, [](const char * symbol, Poincare::Context * context) { return true; }, (char *)m_variables, Poincare::SymbolAbstract::k_maxNameSize, numberOfVariables);
    if (numberOfVariables == -1) {
      return Error::TooManyVariables;
    }
    /* The equation has been parsed so there should be no
     * Error::VariableNameTooLong*/
    assert(numberOfVariables >= 0);
  }

  // Step 2. Linear System?

  // Initialize result
  Expression exactSolutions[k_maxNumberOfExactSolutions];
  Expression exactSolutionsApproximations[k_maxNumberOfExactSolutions];
  EquationStore::Error error;

  bool isLinear = true; // Invalid the linear system if one equation is non-linear
  Preferences::AngleUnit angleUnit = Preferences::sharedPreferences()->angleUnit();
  Preferences::UnitFormat unitFormat = GlobalPreferences::sharedGlobalPreferences()->unitFormat();
  const int nbOfDefinedModels = numberOfDefinedModels();
  assert(nbOfDefinedModels <= k_maxNumberOfEquations);
  {
    /* Create matrix coefficients and vector constants as:
     *   coefficients * (x y z ...) = constants */
    Expression coefficients[k_maxNumberOfEquations][Expression::k_maxNumberOfVariables];
    Expression constants[k_maxNumberOfEquations];
    for (int i = 0; i < nbOfDefinedModels; i++) {
      isLinear = isLinear && simplifiedExpressions[i].getLinearCoefficients((char *)m_variables, Poincare::SymbolAbstract::k_maxNameSize, coefficients[i], &constants[i], context, updatedComplexFormat(context), angleUnit, unitFormat, symbolicComputation);
      if (!isLinear) {
        if (nbOfDefinedModels > 1 || numberOfVariables > 1) {
          return Error::NonLinearSystem;
        } else {
          break;
        }
      }
    }
    if (isLinear) {
      m_degree = 1;
      m_type = Type::LinearSystem;
      error = resolveLinearSystem(exactSolutions, exactSolutionsApproximations, coefficients, constants, context);
    }
    // Clean coefficients and constants from pool allocated memory
  }

  if (!isLinear) {
    // Step 3. Polynomial & Monovariable?
    assert(numberOfVariables == 1 && nbOfDefinedModels == 1);
    Expression polynomialCoefficients[Expression::k_maxNumberOfPolynomialCoefficients];
    m_degree = simplifiedExpressions[0].getPolynomialReducedCoefficients(m_variables[0], polynomialCoefficients, context, updatedComplexFormat(context), angleUnit, unitFormat, symbolicComputation);
    if (m_degree == 2 || m_degree == 3) {
      // Polynomial degree <= 3
      m_type = Type::PolynomialMonovariable;
      error = oneDimensionalPolynomialSolve(exactSolutions, exactSolutionsApproximations, polynomialCoefficients, context);
    } else {
      // Step 4. Monovariable non-polynomial or polynomial with degree > 2
      m_type = Type::Monovariable;
      m_intervalApproximateSolutions[0] = -10;
      m_intervalApproximateSolutions[1] = 10;
      return Error::RequireApproximateSolution;
    }
  }

  // Create the results' layouts
  // Some exam mode configuration requires to display only approximate solutions
  int solutionIndex = 0;
  int initialNumberOfSolutions = m_numberOfSolutions <= k_maxNumberOfExactSolutions ? m_numberOfSolutions : -1;
  // We iterate through the solutions and the potential delta
  for (int i = 0; i < initialNumberOfSolutions; i++) {
    if (exactSolutions[i].isUninitialized()) {
      continue;
    }
    assert(!exactSolutionsApproximations[i].isUninitialized());
    if (exactSolutionsApproximations[i].type() == ExpressionNode::Type::Nonreal) {
      // Discard nonreal solutions.
      m_numberOfSolutions--;
      continue;
    } else if (exactSolutionsApproximations[i].type() == ExpressionNode::Type::Undefined) {
      /* The solution is undefined, which means that the equation contained
       * unreduced undefined terms, such as a sequence or an integral. */
      m_numberOfSolutions--;
      error = Error::EquationUndefined;
      continue;
    }

    m_exactSolutionApproximateLayouts[solutionIndex] = PoincareHelpers::CreateLayout(exactSolutionsApproximations[i], context);
    if (ExpressionDisplayPermissions::ShouldNeverDisplayExactOutput(exactSolutions[i], context)) {
      /* Cheat: declare exact and approximate solutions to be identical in when
       * the exam mode forbids this exact solution to display only the
       * approximate solutions. */
      m_exactSolutionIdentity[solutionIndex] = true;
    } else {
      // Check for identity between exact and approximate layouts
      m_exactSolutionExactLayouts[solutionIndex] = PoincareHelpers::CreateLayout(exactSolutions[i], context);
      char exactBuffer[::Constant::MaxSerializedExpressionSize];
      char approximateBuffer[::Constant::MaxSerializedExpressionSize];
      m_exactSolutionExactLayouts[solutionIndex].serializeForParsing(exactBuffer, ::Constant::MaxSerializedExpressionSize);
      m_exactSolutionApproximateLayouts[solutionIndex].serializeForParsing(approximateBuffer, ::Constant::MaxSerializedExpressionSize);
      m_exactSolutionIdentity[solutionIndex] = strcmp(exactBuffer, approximateBuffer) == 0;
      if (!m_exactSolutionIdentity[solutionIndex]) {
        /* We parse approximateBuffer instead of using directly exactSolutionsApproximations[i] because
         * exactSolutionsApproximations[i] is Float and ExactAndApproximateExpressionsAreEqual
         * is always false for Float. Indeed we Parse like in Calculation (see Calculation::approximateOutput) */
        m_exactSolutionEquality[solutionIndex] = Expression::ExactAndApproximateExpressionsAreEqual(exactSolutions[i], Expression::Parse(approximateBuffer, nullptr));
      }
    }
    solutionIndex++;
  }

  // Assert the remaining layouts had been properly tidied.
  assert(error != Error::NoError
     || solutionIndex >= k_maxNumberOfExactSolutions
     || (m_exactSolutionExactLayouts[solutionIndex].isUninitialized()
      && m_exactSolutionApproximateLayouts[solutionIndex].isUninitialized()));
  return error;
}

EquationStore::Error EquationStore::resolveLinearSystem(Expression exactSolutions[k_maxNumberOfExactSolutions], Expression exactSolutionsApproximations[k_maxNumberOfExactSolutions], Expression coefficients[k_maxNumberOfEquations][Expression::k_maxNumberOfVariables], Expression constants[k_maxNumberOfEquations], Context * context) {
  Preferences::AngleUnit angleUnit = Preferences::sharedPreferences()->angleUnit();
  Preferences::UnitFormat unitFormat = GlobalPreferences::sharedGlobalPreferences()->unitFormat();
  // n unknown variables
  int n = 0;
  while (n < Expression::k_maxNumberOfVariables && m_variables[n][0] != 0) {
    n++;
  }
  int m = numberOfDefinedModels(); // m equations
  /* Create the matrix (A | b) for the equation Ax=b */
  Matrix Ab = Matrix::Builder();
  for (int i = 0; i < m; i++) {
    for (int j = 0; j < n; j++) {
      Ab.addChildAtIndexInPlace(coefficients[i][j], Ab.numberOfChildren(), Ab.numberOfChildren());
    }
    Ab.addChildAtIndexInPlace(constants[i], Ab.numberOfChildren(), Ab.numberOfChildren());
  }
  Ab.setDimensions(m, n+1);

  // Compute the rank of (A | b)
  int rankAb = Ab.rank(context, updatedComplexFormat(context), angleUnit, unitFormat, true);
  if (rankAb == -1) {
    return Error::EquationUndefined;
  }

  // Initialize the number of solutions
  m_numberOfSolutions = INT_MAX;
  /* If the matrix has one null row except the last column, the system is
   * inconsistent (equivalent to 0 = x with x non-null */
  for (int j = m-1; j >= 0; j--) {
    bool rowWithNullCoefficients = true;
    for (int i = 0; i < n; i++) {
      if (Ab.matrixChild(j, i).isNull(context) != TrinaryBoolean::True) {
        rowWithNullCoefficients = false;
        break;
      }
    }
    if (rowWithNullCoefficients && Ab.matrixChild(j, n).isNull(context) != TrinaryBoolean::True) {
      // TODO: Handle ExpressionNode::NullStatus::Unknown
      m_numberOfSolutions = 0;
    }
  }
  if (m_numberOfSolutions > 0) {
    // if rank(A | b) < n, the system has an infinite number of solutions
    if (rankAb == n && n > 0) {
      // Otherwise, the system has n solutions which correspond to the last column
      m_numberOfSolutions = n;
      for (int i = 0; i < m_numberOfSolutions; i++) {
        exactSolutions[i] = Expression();
        Ab.matrixChild(i,n).cloneAndSimplifyAndApproximate(exactSolutions + i, exactSolutionsApproximations + i, context, updatedComplexFormat(context), angleUnit, unitFormat);
      }
    }
  }
  return Error::NoError;
}

EquationStore::Error EquationStore::oneDimensionalPolynomialSolve(Expression exactSolutions[k_maxNumberOfExactSolutions], Expression exactSolutionsApproximations[k_maxNumberOfExactSolutions], Expression coefficients[Expression::k_maxNumberOfPolynomialCoefficients], Context * context) {
  /* Equation ax^2+bx+c = 0 */
  Expression delta;
  bool solutionsAreApproximated = false;
  ReductionContext reductionContext(context, updatedComplexFormat(context), Poincare::Preferences::sharedPreferences()->angleUnit(), Preferences::UnitFormat::Metric, ReductionTarget::User);
  if (m_degree == 2) {
    m_numberOfSolutions = Poincare::Polynomial::QuadraticPolynomialRoots(coefficients[2], coefficients[1], coefficients[0], exactSolutions, exactSolutions + 1, &delta, reductionContext);
  } else {
    assert(m_degree == 3);
    m_numberOfSolutions = Poincare::Polynomial::CubicPolynomialRoots(coefficients[3], coefficients[2], coefficients[1], coefficients[0], exactSolutions, exactSolutions + 1, exactSolutions + 2, &delta, reductionContext, &solutionsAreApproximated);
  }
  exactSolutions[m_numberOfSolutions++] = delta;
  for (int i = 0; i < m_numberOfSolutions; i++) {
    if (solutionsAreApproximated) {
      exactSolutionsApproximations[i] = exactSolutions[i].clone();
    } else {
      Expression exactSolution = exactSolutions[i];
      exactSolutions[i] = Expression();
      exactSolution.cloneAndSimplifyAndApproximate(exactSolutions + i, exactSolutionsApproximations + i, context, updatedComplexFormat(context), Poincare::Preferences::sharedPreferences()->angleUnit(), GlobalPreferences::sharedGlobalPreferences()->unitFormat());
    }
  }
  return Error::NoError;
}

void EquationStore::tidySolution(char * treePoolCursor) {
  for (int i = 0; i < k_maxNumberOfExactSolutions; i++) {
    if (treePoolCursor == nullptr || m_exactSolutionExactLayouts[i].isDownstreamOf(treePoolCursor)) {
      m_exactSolutionExactLayouts[i] = Layout();
      assert(treePoolCursor == nullptr || m_exactSolutionApproximateLayouts[i].isDownstreamOf(treePoolCursor));
      m_exactSolutionApproximateLayouts[i] = Layout();
    }
  }
}

Preferences::ComplexFormat EquationStore::updatedComplexFormat(Context * context) {
  Preferences::ComplexFormat complexFormat = Preferences::sharedPreferences()->complexFormat();
  if (complexFormat == Preferences::ComplexFormat::Real && isExplicitlyComplex(context)) {
    return Preferences::ComplexFormat::Cartesian;
  }
  return complexFormat;
}

bool EquationStore::isExplicitlyComplex(Context * context) {
  int definedModelsTotal = numberOfDefinedModels();
  for (int i = 0; i < definedModelsTotal; i++) {
    // Ignore defined symbols if user variables are not used
    if (modelForRecord(definedRecordAtIndex(i))->containsIComplex(context, m_userVariablesUsed ? SymbolicComputation::ReplaceAllDefinedSymbolsWithDefinition : SymbolicComputation::ReplaceDefinedFunctionsWithDefinitions)) {
      return true;
    }
  }
  return false;
}

}
