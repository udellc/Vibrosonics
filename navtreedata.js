/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "VibroSonics", "index.html", [
    [ "Experience audio through vibration", "index.html#autotoc_md1", null ],
    [ "Table of Contents", "index.html#autotoc_md2", null ],
    [ "Why Vibrosonics Matters", "index.html#autotoc_md3", null ],
    [ "Key Features", "index.html#autotoc_md4", null ],
    [ "Arduino Setup", "index.html#autotoc_md5", [
      [ "Install Arduino IDE and ESP32 Board Support", "index.html#autotoc_md6", null ],
      [ "Set Board and Ports", "index.html#autotoc_md7", null ],
      [ "Add Libraries", "index.html#autotoc_md8", null ],
      [ "Upload a Sketch", "index.html#autotoc_md9", null ]
    ] ],
    [ "Web Development Setup", "index.html#autotoc_md10", [
      [ "Install Node.js and npm", "index.html#autotoc_md11", null ],
      [ "Install Dependencies", "index.html#autotoc_md12", null ],
      [ "Start the Development Server", "index.html#autotoc_md13", null ],
      [ "Additional Commands", "index.html#autotoc_md14", null ],
      [ "Troubleshooting", "index.html#autotoc_md15", null ]
    ] ],
    [ "ESP32 Web Server Setup", "index.html#autotoc_md16", [
      [ "Setup Upload Mode (Necessary if web app is not on the ESP32, otherwise skip)", "index.html#autotoc_md17", null ],
      [ "Connect to the Web App", "index.html#autotoc_md18", null ]
    ] ],
    [ "Developer Notes", "index.html#autotoc_md19", [
      [ "Library Architecture", "index.html#autotoc_md20", null ],
      [ "Web App Architecture", "index.html#autotoc_md21", [
        [ "System Overview", "index.html#autotoc_md22", null ],
        [ "Architecture Components", "index.html#autotoc_md23", [
          [ "Frontend (Web App)", "index.html#autotoc_md24", null ],
          [ "Backend (ESP32 Web Server)", "index.html#autotoc_md25", null ],
          [ "Inter-Core Communication", "index.html#autotoc_md26", null ],
          [ "Audio Analysis Loop (Core 1)", "index.html#autotoc_md27", null ]
        ] ],
        [ "Data Flow", "index.html#autotoc_md28", null ],
        [ "Key Design Decisions", "index.html#autotoc_md29", null ]
      ] ],
      [ "API Classes", "index.html#autotoc_md30", null ]
    ] ],
    [ "Examples", "index.html#autotoc_md31", null ],
    [ "Contributors", "index.html#autotoc_md32", [
      [ "2025-26 Software Team", "index.html#autotoc_md33", null ],
      [ "2024-25 Software Team", "index.html#autotoc_md34", null ],
      [ "Special Thanks", "index.html#autotoc_md35", null ]
    ] ],
    [ "Contributing Guide", "md__c_o_n_t_r_i_b_u_t_i_n_g.html", [
      [ "Table of Contents", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md37", null ],
      [ "Code of Conduct", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md38", null ],
      [ "Getting Started", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md39", null ],
      [ "Branching & Workflow", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md40", null ],
      [ "Issues & Planning", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md41", null ],
      [ "Commit Messages", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md42", null ],
      [ "Code Style, Linting & Formatting", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md43", null ],
      [ "Testing", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md44", null ],
      [ "Pull Requests & Reviews", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md45", null ],
      [ "CI/CD", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md46", null ],
      [ "Security & Secrets", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md47", null ],
      [ "Documentation Expectations", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md48", null ],
      [ "Release Process", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md49", null ],
      [ "Support & Contact", "md__c_o_n_t_r_i_b_u_t_i_n_g.html#autotoc_md50", null ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", null ],
        [ "Functions", "functions_func.html", null ],
        [ "Variables", "functions_vars.html", null ],
        [ "Related Symbols", "functions_rela.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Variables", "globals_vars.html", null ],
        [ "Enumerations", "globals_enum.html", null ],
        [ "Enumerator", "globals_eval.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"_grain_8cpp.html"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';