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
    <div className="p-4 flex w-full h-[60px] border-b-2 border-black justify-between">

      {/* Left side */}
      <div className="flex flex-row justify-between">

        {/* Logo can go here */}
        {/* TODO: use the vibrosonics logo file in an assets folder under WebApp/assets and link img here */}
        <h2 className="font-bold text-3xl ml-2">
          Vibrosonics
        </h2>
      </div>

      {/* Right side */}
      <div>
        {/* The tabs will go here */}
      </div>
    </div>
  );
};

export default Header;
