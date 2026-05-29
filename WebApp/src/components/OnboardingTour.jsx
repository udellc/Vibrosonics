/***************************************************************
 * File: configManager.jsx
 *
 * Date: 05/28/2026
 *
 * Description: Component for the onboarding tour walkthrough
 *
 * Author: Bella Mann
 ***************************************************************/

import { useCallback, useEffect, useState } from "preact/hooks";
import { route } from "preact-router";
import steps from "../data/onboardingTourData.json";

const TOUR_STORAGE_KEY = "vibrosonics-onboarding-tour-complete";

const OnboardingTour = () => {
  const [isRunning, setIsRunning] = useState(false);
  const [currentStepIndex, setCurrentStepIndex] = useState(0);

  /** @type {[DOMRect | null, (val: DOMRect | null) => void]} */
const [targetRect, setTargetRect] = useState(null);

  const currentStep = steps[currentStepIndex];

  const updateTargetRect = useCallback(() => {
    if (!currentStep.selector) return setTargetRect(null);

    const target = document.querySelector(currentStep.selector);

    setTargetRect(target ? target.getBoundingClientRect() : null);
  }, [currentStep]);

  const finishTour = () => {
    localStorage.setItem(TOUR_STORAGE_KEY, "true");
    setIsRunning(false);
  };

  const restartTour = () => {
    localStorage.removeItem(TOUR_STORAGE_KEY);
    setCurrentStepIndex(0);
    setIsRunning(true);
  };

  useEffect(() => {
     if(localStorage.getItem(TOUR_STORAGE_KEY) === "true")
      setIsRunning(true);
  }, []);

  useEffect(() => {
    if (!isRunning || !currentStep) return;

    route(currentStep.route, false);

    const sync = () => updateTargetRect();
    const timer = setTimeout(sync, 100);

    window.addEventListener("resize", sync);
    window.addEventListener("scroll", sync, true);

    return () => {
      clearTimeout(timer);
      window.removeEventListener("resize", sync);
      window.removeEventListener("scroll", sync, true);
    };
  }, [isRunning, currentStepIndex, updateTargetRect, currentStep]);

  const getDynamicStyles = () => {
    /** @type {DOMRect} */
    const rect = (/** @type {any} */ (targetRect));

    if (!rect) return { top: "50%", left: "50%", transform: "translate(-50%, -50%)" };

    // Now 'rect' is guaranteed to be DOMRect, not null/never
    let top = rect.bottom + 16;
    let left = Math.max(16, rect.left);

    if (currentStep.placement === "right") {
      top = rect.top;
      left = rect.right + 24;
    } else if (currentStep.placement === "left") {
      top = rect.top;
      left = rect.left - 410;
    } else if (currentStep.placement === "center") {
      return { top: "30%", left: "calc(50% - 190px)" };
    }
      // Boundary protection
      top = Math.min(top, window.innerHeight - 250);
      left = Math.min(Math.max(16, left), window.innerWidth - 410);
      return { top: `${top}px`, left: `${left}px` };
  };

  return (
    <div id="onboarding-tour-container">
      {!isRunning ? (
        /* 1. Only show Restart Button if NOT running */
        <button
          onClick={restartTour}
          className="fixed bottom-4 right-4 z-[60] bg-gray-400 text-white px-4 py-2 rounded-lg font-bold shadow-lg hover:bg-slate-700 transition-all"
        >
          Restart Tour
        </button>
      ) : (
        <>
          <div className="fixed inset-0 bg-black/50 z-40 pointer-events-none" />
          
          {targetRect && (
            <div
              className="fixed z-50 border-4 border-amber-400 rounded-xl pointer-events-none shadow-[0_0_0_9999px_rgba(0,0,0,0.45)] transition-all duration-300"
              style={{
                top: `${targetRect.top - 8}px`,
                left: `${targetRect.left - 8}px`,
                width: `${targetRect.width + 16}px`,
                height: `${targetRect.height + 16}px`,
              }}
            />
          )}

          <div
            className="fixed z-50 bg-white rounded-xl shadow-2xl border border-slate-200 p-5 w-[380px] transition-all duration-300"
            style={getDynamicStyles()}
          >
            <h2 className="font-bold text-xl mb-2 text-slate-900">{currentStep?.title}</h2>
            <p className="text-slate-600 leading-relaxed mb-6">{currentStep?.text}</p>

            <div className="flex items-center justify-between">
              <span className="text-xs font-bold text-slate-400 uppercase tracking-widest">
                {currentStepIndex + 1} / {steps.length}
              </span>
              <div className="flex gap-2">
                <button onClick={finishTour} className="px-3 py-1 text-sm text-slate-400 hover:text-slate-600">Skip</button>
                <button 
                  disabled={currentStepIndex === 0}
                  onClick={() => { setTargetRect(null); setCurrentStepIndex(i => i - 1); }}
                  className="px-3 py-1 text-sm rounded border border-slate-200 disabled:opacity-20"
                >
                  Back
                </button>
                <button
                  onClick={() => currentStepIndex === steps.length - 1 ? finishTour() : (setTargetRect(null), setCurrentStepIndex(i => i + 1))}
                  className="px-4 py-1 text-sm bg-amber-400 hover:bg-amber-500 rounded font-bold shadow-sm transition-colors"
                >
                  {currentStepIndex === steps.length - 1 ? "Finish" : "Next"}
                </button>
              </div>
            </div>
          </div>
        </>
      )}
    </div>
  );
};

export default OnboardingTour;