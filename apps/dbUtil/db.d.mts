export default db;
declare const db: Db;
declare class Db {
    db: any;
    MySQL: typeof MySQL;
    Sqlite: typeof Sqlite;
    getSqlDateTime(date: any): string;
}
declare class MySQL {
    static loadSchema(db: any, table: any): Table;
}
declare class Sqlite {
    static loadSchema(db: any, table: any): Table;
}
declare class Table {
    constructor(db_: any, tableName: any, sqlite: any);
    name: any;
    columns: any[];
    cols: {};
    indexes: any[];
    keys: {};
    fkeys: any[];
    db: any;
    sqlite: boolean;
    has(col: any): boolean;
    addColumn(name: any, type: any, extra: any): void;
    loadColumns(db: any): void;
    loadSqliteColumns(db: any): void;
}
