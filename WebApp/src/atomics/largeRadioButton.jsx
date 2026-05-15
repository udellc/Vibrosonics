/***************************************************************
 * File: largeRadioButton.jsx
 *
 * Date: 5/14/2026
 *
 * Description: UI radio button displaying title and description
 *
 * Author: Ivan Wong
 ***************************************************************/

/**
 * @brief Provides an accessible custom radio button which displays title and description
 * 
 * @param {Object} _ - Expanded object containing actual params
 * @param {String} _.label - Title of button
 * @param {String} _.description - Description of button
 * @param {Boolean} _.checked - Toggled state
 * @param {CallableFunction} _.onChange - Invoked function when changed
 * @param {any} _.value - Current value of button
 * 
 * @returns Instance of custom radio button
 */
const LargeRadioButton = ({ label, description, checked, onChange, value }) => (
  <label className="flex items-start gap-4 p-5 rounded-2xl border border-gray-400 cursor-pointer hover:border-black">
    <div className={`w-8 h-8 rounded-full border-2 flex items-center justify-center mt-0.5 ${checked ? 'border-blue-600' : 'border-gray-300'}`}>
      <div className={`w-4 h-4 rounded-full ${checked ? 'bg-blue-600' : 'bg-transparent'}`}></div>
    </div>
    <div className="grow">
      <div className={`text-xl font-semibold text-gray-900`}>{label}</div>
      <p className="text-gray-600 text-lg">{description}</p>
    </div>
    <input
      type="radio"
      name="connectionMode"
      value={value}
      checked={checked}
      onChange={() => onChange(value)}
      className="sr-only" // screen reader only, hidden but accessible
    />
  </label>
);

export default LargeRadioButton;
