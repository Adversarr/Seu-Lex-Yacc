
  // syntax
  sly::core::grammar::ContextFreeGrammar cfg(productions, start_syntax_token, ending);
  sly::core::grammar::Lr1 lr1;
  cfg.Compile(lr1);
  table = cfg.GetLrTable();

  // rewrite
  ofstream outputFile("../test/out_precompile.cpp");
  table.PrintGeneratorCode(outputFile);
  outputFile.close();

  // return 0;
  