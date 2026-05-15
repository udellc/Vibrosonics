/***************************************************************
 * File: loadingSPinner.jsx
 *
 * Date: 5/14/2026
 *
 * Description: Loading spinner component
 *
 * Author: Bella Mann
 ***************************************************************/

/**
 * @brief Displays a custom loading spinner
 *
 * @param {Object} _ - Expanded object containing params
 * @param {Number} _.size - Size of the spinner
 * @param {String} _.label - Text for the spinner
 *
 * @returns A loading spinner UI component
 */
export default function LoadingSpinner({ size, label }) {
  const sizeStyle = `w-${size} h-${size}`;

  return (
    <div className="flex flex-col items-center gap-3">
      <div
        className={`${sizeStyle} border-4 border-white border-t-amber-500 rounded-full animate-spin`}
      />
      <p className="text-black font-medium animate-pulse text-lg">{label}</p>
    </div>
  );
}
