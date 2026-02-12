import { useState } from "preact/hooks";

export const createProject = (name, libraryLength, currentValues) => {
  return {
    id: Date.now(),
    name: name || `Project ${libraryLength + 1}`,
    data: {
      knobValue: { ...currentValues.knobValue },
      sliderValue: { ...currentValues.sliderValue },
      isAdvanced: currentValues.isAdvanced,
      activeGenre: currentValues.activeGenre
    }
  };
};

