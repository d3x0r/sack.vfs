tsc %1 --target esnext --declaration --allowJs --emitDeclarationOnly
:npx -p typescript tsc src/**/*.js --declaration --allowJs --emitDeclarationOnly --outDir types
