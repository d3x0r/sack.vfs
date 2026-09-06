call tsc %1 --target esnext --skipLibCheck --declaration --allowJs --emitDeclarationOnly
:npx -p typescript tsc src/**/*.js --declaration --allowJs --emitDeclarationOnly --outDir types
