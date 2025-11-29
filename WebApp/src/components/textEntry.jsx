/***************************************************************
 * File: textEntry.jsx
 *
 * Date: 11/23/2025
 *
 * Description: TBD
 *
 * Author: Ivan Wong
 ***************************************************************/


/**
 * @brief The text entry component provides a generic component for a mutable text interface
 *
 * @param { Object } _ - Expanded object for text entry params
 * @param { String } _.label - Label for the text entry
 * @param { String } _.entryType - Type of input to be used
 * @param { String } _.presetText - Text displayed in the text entry before a user enters anything
 * @param { CallableFunction } _.onChange - Callback to be executed once the user submits the text entry
 */
const TextEntry = ({ label, entryType, presetText, onChange }) => {
  const handleInput = (e) => {
    const text = String(e.currentTarget.value);
    onChange(text);
  };

  return (
    <div>
      <label>
        {`${label}: `}
        <input
          type={entryType}
          placeholder={presetText}
          onChange={handleInput}
        />
      </label>
    </div>
  );
};

export default TextEntry;
