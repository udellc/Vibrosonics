/***************************************************************
 * File: modulesPage.jsx
 *
 * Date: 11/22/2025
 *
 * Description: The audio analysis modules page for reconfiguring
 * haptic feedback
 *
 * Author: Ivan Wong
 ***************************************************************/

import AnalysisModule from "../components/analysisModule";

const ModulesPage = () => {
  return (
    <div>
      <h1 className="font-bold mt-10">
        Modules page
        <AnalysisModule />
      </h1>
    </div>
  );
};

export default ModulesPage;
