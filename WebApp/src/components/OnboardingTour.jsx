import { useEffect, useState } from "preact/hooks";
import { route } from "preact-router";

const TOUR_STORAGE_KEY = "vibrosonics-onboarding-tour-complete";

const steps = [
  {
    route: "/",
    selector: "#welcomeheading",
    placement: "right",
    title: "Welcome!",
    text: "This is the Home Page, where you can learn more information about the software.",
  },
  {
    route: "/",
    selector: "#main-nav",
    title: "Navigation Bar",
    text: "Use the navigation bar to move to Networks, Modules, FM Radio, and more!",
  },
  {
    route: "/network",
    selector: "#scan-networks-button",
    title: "Scan Networks",
    text: "Use this button to scan for available networks before connecting the Vibrosonics device.",
  },
  {
    route: "/modules",
    selector: "#modules-button",
    title: "EQ Presets",
    text: "On this tab, you can configure the device to correlate with the genre of music you are listening to.",
  },
  {
    route: "/modules",
    selector: "#modules-button",
    title: "Bass and Major Peak Frequency Configuration",
    text: "Lower down in the modules tab you can adjust the percussion and major peaks modules. Click next to learn more about what the knobs control.",
  },
  {
    route: "/modules",
    selector: "#modules-button",
    title: "Entropy Configuration",
    text: "This helps filter out messy noise so the device reacts more to meaningful sound.",
  },
  {
    route: "/modules",
    selector: "#modules-button",
    title: "Low Cut Frequency and High Cut Frequency Configuration:",
    text: "Low Cut Frequency sets the lowest sounds the module pays attention to. High Cut Frequency sets the highest sounds.",
  },
  {
    route: "/",
    selector: '#main-nav a[href="/radio"]',
    title: "Additional Tabs",
    text: "Check out the FM Radio tab to connect the device to a radio station.",
  },
  {
    route: "/",
    selector: '#main-nav a[href="https://www.youtube.com/@cymaspace"]',
    title: "Additional Tabs",
    text: "Check out the CymaSpace tab to learn more about CymaSpace.",
  },
  {
    route: "/",
    selector: '#main-nav a[href="https://github.com/udellc/Vibrosonics"]',
    title: "Additional Tabs",
    text: "Check out the Github Repo tab to learn more about the source code on Github.",
  },
  {
    route: "/network",
    selector: "#scan-networks-button",
    title: "Scan Networks",
    text: "Use this button to scan for available networks before connecting the Vibrosonics device.",
  },
  {
    route: "/network",
    selector: "#scan-networks-button",
    title: "Scan Networks",
    text: "Use this button to scan for available networks before connecting the Vibrosonics device.",
  },
  {
    route: "/network",
    selector: "#scan-networks-button",
    title: "Scan Networks",
    text: "Use this button to scan for available networks before connecting the Vibrosonics device.",
  },
  {
    route: "/",
    selector: "#welcomeheading",
    placement: "right",
    title: "End of Tour!",
    text: "",
  },
];

/**
 * Rough-draft walkthrough.

 */
const OnboardingTour = () => {
  const [isRunning, setIsRunning] = useState(false);
  const [currentStepIndex, setCurrentStepIndex] = useState(0);

  /** @type {[DOMRect | null, Function]} */
  const [targetRect, setTargetRect] = useState(null);

  const currentStep = steps[currentStepIndex];

  useEffect(() => {
    const hasCompletedTour =
      localStorage.getItem(TOUR_STORAGE_KEY) === "true";

    if (!hasCompletedTour) {
      setIsRunning(true);
    }
  }, []);

  useEffect(() => {
    if (!isRunning || !currentStep) return;

    route(currentStep.route, false);

    const timer = setTimeout(() => {
      updateTargetRect();
    }, 100);

    return () => clearTimeout(timer);
  }, [isRunning, currentStepIndex]);

  useEffect(() => {
    if (!isRunning) return;

    const handleReposition = () => updateTargetRect();

    window.addEventListener("resize", handleReposition);
    window.addEventListener("scroll", handleReposition, true);

    return () => {
      window.removeEventListener("resize", handleReposition);
      window.removeEventListener("scroll", handleReposition, true);
    };
  }, [isRunning, currentStepIndex]);

  const updateTargetRect = () => {
    if (!currentStep.selector) {
      setTargetRect(null);
      return;
   }

  const target = document.querySelector(currentStep.selector);

    if (!target) {
      setTargetRect(null);
      return;
    }

    setTargetRect(target.getBoundingClientRect());
  };

  const finishTour = () => {
    localStorage.setItem(TOUR_STORAGE_KEY, "true");
    setIsRunning(false);
  };

  const restartTour = () => {
    localStorage.removeItem(TOUR_STORAGE_KEY);
    setCurrentStepIndex(0);
    setIsRunning(true);
  };

  const goNext = () => {
  if (currentStepIndex === steps.length - 1) {
    finishTour();
    return;
  }

  setTargetRect(null);
  setCurrentStepIndex(currentStepIndex + 1);
  };

  const goBack = () => {
  if (currentStepIndex === 0) return;

  setTargetRect(null);
  setCurrentStepIndex(currentStepIndex - 1);
  };

  if (!isRunning || !currentStep) {
    return (
      <button
        onClick={restartTour}
        className="fixed bottom-4 right-4 z-50 bg-slate-800 text-white px-4 py-2 rounded-lg font-bold shadow-lg"
      >
        Restart Tour
      </button>
    );
  }

  if (!targetRect) {
  return (
    <>
      <div className="fixed inset-0 bg-black/50 z-40 pointer-events-none" />

      <div
        className="fixed z-50 pointer-events-none shadow-[0_0_0_9999px_rgba(0,0,0,0.45)]"
        style={{
          top: "0px",
          left: "0px",
          width: "1px",
          height: "1px",
        }}
      />
    </>
  );
  }

  /** @type {DOMRect} */
  const rect = targetRect;

  let cardTop =
    rect.bottom + 16 > window.innerHeight - 230
        ? Math.max(16, rect.top - 210)
        : rect.bottom + 16;

  let cardLeft =
    rect.left + 390 > window.innerWidth
        ? Math.max(16, window.innerWidth - 410)
        : Math.max(16, rect.left);

  if (currentStep.placement === "right") {
    cardTop = Math.max(16, rect.top);
    cardLeft = Math.min(window.innerWidth - 410, rect.right + 24);
  }

  if (currentStep.placement === "left") {
    cardTop = Math.max(16, rect.top);
    cardLeft = Math.max(16, rect.left - 410);
   }

if (currentStep.placement === "center") {
  cardTop = Math.max(24, window.innerHeight * 0.28);
  cardLeft = Math.max(16, window.innerWidth / 2 - 190);
}

  return (
    <>
      <div className="fixed inset-0 bg-black/50 z-40 pointer-events-none" />

      <div
        className="fixed z-50 border-4 border-amber-400 rounded-xl pointer-events-none shadow-[0_0_0_9999px_rgba(0,0,0,0.45)]"
        style={{
          top: `${rect.top - 8}px`,
          left: `${rect.left - 8}px`,
          width: `${rect.width + 16}px`,
          height: `${rect.height + 16}px`,
        }}
      />

      <div
        className="fixed z-50 bg-white rounded-xl shadow-2xl border border-slate-200 p-4 w-[380px]"
        style={{
          top: `${cardTop}px`,
          left: `${cardLeft}px`,
        }}
      >
        <h2 className="font-bold text-xl mb-2">{currentStep.title}</h2>

        <p className="text-slate-700 leading-relaxed mb-4">
          {currentStep.text}
        </p>

        {!targetRect && (
          <p className="text-sm text-red-600 mb-3">
            Target not found yet. This step may need its selector updated.
          </p>
        )}

        <div className="flex items-center justify-between">
          <span className="text-sm text-slate-500">
            {currentStepIndex + 1}/{steps.length}
          </span>

          <div className="flex gap-2">
            <button
              onClick={finishTour}
              className="px-3 py-2 rounded-md border border-slate-300"
            >
              Skip
            </button>

            <button
              onClick={goBack}
              disabled={currentStepIndex === 0}
              className="px-3 py-2 rounded-md border border-slate-300 disabled:opacity-40"
            >
              Back
            </button>

            <button
              onClick={goNext}
              className="px-3 py-2 rounded-md bg-amber-400 border border-amber-500 font-bold"
            >
              {currentStepIndex === steps.length - 1 ? "Finish" : "Next"}
            </button>
          </div>
        </div>
      </div>
    </>
  );
};

export default OnboardingTour;