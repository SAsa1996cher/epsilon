// SPDX-License-Identifier: CC-BY-NC-ND-4.0
// Caution: Dutch exam mode is subject to a compliance certification by a government agency. Distribution of a non-certified Dutch exam mode is illegal.

#include "exam_mode_configuration.h"
#include "global_preferences.h"

using namespace Poincare;

int ExamModeConfiguration::numberOfAvailableExamMode() {
  /* Available exam mode depends on the selected country and the active mode.
   * A user could first activate an exam mode and then change the country. */
  Preferences::ExamMode examMode = Preferences::sharedPreferences()->examMode();
  if (examMode == Preferences::ExamMode::Standard
      || examMode == Preferences::ExamMode::Dutch
      || examMode == Preferences::ExamMode::IBTest
      || examMode == Preferences::ExamMode::Portuguese
      || examMode == Preferences::ExamMode::English) {
    // Reactivation button
    return 1;
  }
  CountryPreferences::AvailableExamModes availableExamModes = GlobalPreferences::sharedGlobalPreferences()->availableExamModes();

  if (availableExamModes == CountryPreferences::AvailableExamModes::PressToTestOnly
      || examMode == Preferences::ExamMode::PressToTest) {
    assert(examMode == Preferences::ExamMode::Off || examMode == Preferences::ExamMode::PressToTest);
    // Menu shouldn't be visible
    return 0;
  }
  // Activation button(s)
  assert(examMode == Preferences::ExamMode::Off);
  if (availableExamModes == CountryPreferences::AvailableExamModes::StandardOnly || availableExamModes == CountryPreferences::AvailableExamModes::PortugueseOnly || availableExamModes == CountryPreferences::AvailableExamModes::EnglishOnly) {
    return 1;
  }
  if (availableExamModes == CountryPreferences::AvailableExamModes::StandardAndDutch) {
    return 2;
  }
  assert(availableExamModes == CountryPreferences::AvailableExamModes::All);
  return 5;
}

bool ExamModeConfiguration::pressToTestExamModeAvailable() {
  Preferences::ExamMode examMode = Preferences::sharedPreferences()->examMode();
  CountryPreferences::AvailableExamModes availableExamModes = GlobalPreferences::sharedGlobalPreferences()->availableExamModes();
  return examMode == Preferences::ExamMode::PressToTest
         || ((availableExamModes == CountryPreferences::AvailableExamModes::PressToTestOnly
              || availableExamModes == CountryPreferences::AvailableExamModes::All)
             && examMode == Preferences::ExamMode::Off);
}

bool ExamModeConfiguration::testModeAvailable() {
  return Preferences::sharedPreferences()->examMode()
             == Preferences::ExamMode::Off
         && GlobalPreferences::sharedGlobalPreferences()->availableExamModes()
                == CountryPreferences::AvailableExamModes::All;
}

Preferences::ExamMode ExamModeConfiguration::examModeAtIndex(size_t index) {
  if (GlobalPreferences::sharedGlobalPreferences()->availableExamModes() == CountryPreferences::AvailableExamModes::PortugueseOnly) {
    assert(index == 0);
    return Preferences::ExamMode::Portuguese;
  }
  if (GlobalPreferences::sharedGlobalPreferences()->availableExamModes() == CountryPreferences::AvailableExamModes::EnglishOnly) {
    assert(index == 0);
    return Preferences::ExamMode::English;
  }
  Preferences::ExamMode examModes[] = {Preferences::ExamMode::Standard, Preferences::ExamMode::Dutch, Preferences::ExamMode::Portuguese, Preferences::ExamMode::English, Preferences::ExamMode::IBTest};
  assert(index < sizeof(examModes)/sizeof(Preferences::ExamMode));
  return examModes[index];
}

I18n::Message ExamModeConfiguration::examModeActivationMessage(size_t index) {
  Preferences::ExamMode examMode = Preferences::sharedPreferences()->examMode();
  /* If the country has all exam mode, we specify which one will be reactivated.
   * The country might still have been updated by the user after activation. */
  bool specifyFrenchExamModeType = GlobalPreferences::sharedGlobalPreferences()->availableExamModes() == CountryPreferences::AvailableExamModes::All;
  assert(examMode == Preferences::ExamMode::Off || index == 0);
  switch (examMode) {
    case Preferences::ExamMode::Standard:
      return (specifyFrenchExamModeType ? I18n::Message::ReactivateFrenchExamMode : I18n::Message::ReactivateExamMode);
    case Preferences::ExamMode::Dutch:
      return I18n::Message::ReactivateDutchExamMode;
    case Preferences::ExamMode::IBTest:
      return I18n::Message::ReactivateIBExamMode;
    case Preferences::ExamMode::Portuguese:
      return I18n::Message::ReactivatePortugueseExamMode;
    case Preferences::ExamMode::English:
      return I18n::Message::ReactivateEnglishExamMode;
    default:
      assert(examMode == Preferences::ExamMode::Off);
      switch (examModeAtIndex(index)) {
      case Preferences::ExamMode::Standard:
        return (specifyFrenchExamModeType ? I18n::Message::ActivateFrenchExamMode : I18n::Message::ActivateExamMode);
      case Preferences::ExamMode::Dutch:
        return I18n::Message::ActivateDutchExamMode;
      case Preferences::ExamMode::Portuguese:
        return I18n::Message::ActivatePortugueseExamMode;
      case Preferences::ExamMode::English:
        return I18n::Message::ActivateEnglishExamMode;
      case Preferences::ExamMode::IBTest:
        return I18n::Message::ActivateIBExamMode;
      default:
        assert(false);
        return I18n::Message::Default;
      }
  }
}

I18n::Message ExamModeConfiguration::examModeActivationWarningMessage(Preferences::ExamMode mode) {
  if (mode == Preferences::ExamMode::Off) {
    if (Preferences::sharedPreferences()->examMode() == Preferences::ExamMode::PressToTest) {
      return I18n::Message::ExitPressToTestExamMode;
    }
    return I18n::Message::ExitExamMode;
  } else if (mode == Preferences::ExamMode::Standard) {
    if (Ion::Authentication::clearanceLevel() == Ion::Authentication::ClearanceLevel::NumWorks) {
      return I18n::Message::ActiveExamModeMessage;
    } else {
      return I18n::Message::ActiveExamModeWithResetMessage;
    }
  } else if (mode == Preferences::ExamMode::IBTest) {
    if (Ion::Authentication::clearanceLevel() == Ion::Authentication::ClearanceLevel::NumWorks) {
      return I18n::Message::ActiveIBExamModeMessage;
    } else {
      return I18n::Message::ActiveIBExamModeWithResetMessage;
    }
  } else if (mode == Preferences::ExamMode::PressToTest) {
    if (Ion::Authentication::clearanceLevel() == Ion::Authentication::ClearanceLevel::NumWorks) {
      return I18n::Message::ActivePressToTestModeMessage;
    } else {
      return I18n::Message::ActivePressToTestWithResetMessage;
    }
  } else if (mode == Preferences::ExamMode::Dutch) {
    if (Ion::Authentication::clearanceLevel() == Ion::Authentication::ClearanceLevel::NumWorks) {
      return I18n::Message::ActiveDutchExamModeMessage;
    } else {
      return I18n::Message::ActiveDutchExamModeWithResetMessage;
    }
  } else if (mode == Preferences::ExamMode::English) {
    if (Ion::Authentication::clearanceLevel() == Ion::Authentication::ClearanceLevel::NumWorks) {
      return I18n::Message::ActiveEnglishExamModeMessage;
    } else {
      return I18n::Message::ActiveEnglishExamModeWithResetMessage;
    }
  } else {
    assert(mode == Preferences::ExamMode::Portuguese);
    if (Ion::Authentication::clearanceLevel() == Ion::Authentication::ClearanceLevel::NumWorks) {
      return I18n::Message::ActivePortugueseExamModeMessage;
    } else {
      return I18n::Message::ActiveExamModeWithResetMessage;
    }
  }
}

I18n::Message ExamModeConfiguration::forbiddenAppMessage(Preferences::ExamMode mode, int line) {
  if (mode == Preferences::ExamMode::PressToTest) {
    I18n::Message messages[] = {I18n::Message::ForbiddenAppInPressToTestMode1, I18n::Message::ForbiddenAppInPressToTestMode2};
    return messages[line];
  }
  assert(mode != Preferences::ExamMode::Off);
  I18n::Message messages[] = {I18n::Message::ForbiddenAppInExamMode1, I18n::Message::ForbiddenAppInExamMode2};
  return messages[line];
}

KDColor ExamModeConfiguration::examModeColor(Preferences::ExamMode mode) {
  switch (mode) {
  case Preferences::ExamMode::Standard:
    return k_standardLEDColor;
  case Preferences::ExamMode::Dutch:
    return k_dutchLEDColor;
  case Preferences::ExamMode::Portuguese:
    return k_portugueseLEDColor;
  default:
    return KDColorBlack;
  }
}

bool ExamModeConfiguration::appIsForbidden(I18n::Message appName) {
  Preferences::ExamMode mode = Preferences::sharedPreferences()->examMode();
  bool pythonDutchExam = appName == I18n::Message::CodeApp && mode == Preferences::ExamMode::Dutch;
  bool elementsForbidden = appName == I18n::Message::ElementsApp && (mode == Preferences::ExamMode::Portuguese || mode == Preferences::ExamMode::Dutch || mode == Preferences::ExamMode::English || Preferences::sharedPreferences()->elementsAppIsForbidden());
  bool solverForbidden = appName == I18n::Message::SolverApp && Preferences::sharedPreferences()->equationSolverIsForbidden();
  return pythonDutchExam || elementsForbidden || solverForbidden;
}

static bool isPrimeFactorization(Expression expression) {
  /* A prime factorization can only be built with integers, powers of integers,
   * and a multiplication. */
  return !expression.recursivelyMatches(
    [](const Expression e, Context * context) {
      return e.isUninitialized()
             || !(e.type() == ExpressionNode::Type::BasedInteger
                  || e.type() == ExpressionNode::Type::Multiplication
                  || (e.type() == ExpressionNode::Type::Power
                      && e.childAtIndex(0).type() == ExpressionNode::Type::BasedInteger
                      && e.childAtIndex(1).type() == ExpressionNode::Type::BasedInteger));
    });
}

bool ExamModeConfiguration::exactExpressionIsForbidden(Expression e) {
  if (!Preferences::sharedPreferences()->exactResultsAreForbidden()) {
    return false;
  }
  bool isFraction = e.type() == ExpressionNode::Type::Division && e.childAtIndex(0).isNumber() && e.childAtIndex(1).isNumber();
  return !(e.isNumber() || isFraction || isPrimeFactorization(e));
}

bool ExamModeConfiguration::additionalResultsAreForbidden() {
  Preferences::ExamMode mode = Preferences::sharedPreferences()->examMode();
  return mode == Preferences::ExamMode::Dutch || mode == Preferences::ExamMode::IBTest;
}

bool ExamModeConfiguration::lineDetailsAreForbidden() {
  return Preferences::sharedPreferences()->examMode() == Preferences::ExamMode::IBTest;
}

bool ExamModeConfiguration::inequalityGraphingIsForbidden() {
  return Preferences::sharedPreferences()->inequalityGraphingIsForbidden();
}

bool ExamModeConfiguration::implicitPlotsAreForbidden() {
  return Preferences::sharedPreferences()->implicitPlotsAreForbidden();
}

bool ExamModeConfiguration::statsDiagnosticsAreForbidden() {
  return Preferences::sharedPreferences()->statsDiagnosticsAreForbidden();
}

bool ExamModeConfiguration::vectorNormIsForbidden() {
  return Preferences::sharedPreferences()->vectorNormIsForbidden();
}

bool ExamModeConfiguration::vectorProductsAreForbidden() {
  return Preferences::sharedPreferences()->vectorProductsAreForbidden();
}

bool ExamModeConfiguration::basedLogarithmIsForbidden() {
  return Preferences::sharedPreferences()->basedLogarithmIsForbidden();
}

bool ExamModeConfiguration::sumIsForbidden() {
  return Preferences::sharedPreferences()->sumIsForbidden();
}

bool ExamModeConfiguration::unitsAreForbidden() {
  Preferences::ExamMode mode = Preferences::sharedPreferences()->examMode();
  return mode == Preferences::ExamMode::Dutch || mode == Preferences::ExamMode::IBTest;
}

I18n::Message ExamModeConfiguration::examModeTitleBarMessage(Poincare::Preferences::ExamMode mode) {
  switch (mode) {
  case Preferences::ExamMode::English:
    return I18n::Message::ExamModeTitleBarUK;
  case Preferences::ExamMode::Dutch:
    return I18n::Message::ExamModeTitleBarNL;
  case Preferences::ExamMode::Portuguese:
    return I18n::Message::ExamModeTitleBarPT;
  default:
    return I18n::Message::Default;
  }
}
