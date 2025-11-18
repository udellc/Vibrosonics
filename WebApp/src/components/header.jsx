/***************************************************************
 * File: header.jsx
 *
 * Date: 11/18/2025
 *
 * Description: The header component for the web app.
 *
 * Author: Ivan Wong
 ***************************************************************/

/**
 * @brief
 *
 * @returns
 */
const Header = () => {
  return (
    <div className="bg-red-600 pt-[15px] pl-[15px] flex w-full h-[60px]">
      <svg width="50" height="50">
        <circle
          cx="20"
          cy="20"
          r="20"
          stroke="gray"
          stroke-width="2"
          fill="white"
        />
        <text fill="#00000" font-size="10" font-family="Verdana" x="5" y="22">Logo?</text>
      </svg>
      <h1>Top bar for spacing and other global buttons</h1>
    </div>
  );
};

export default Header;
