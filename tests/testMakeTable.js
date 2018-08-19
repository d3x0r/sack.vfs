
var sack = require( ".." );
//sack.Volume().unlink( "makeTable.db" );
var db = sack.Sqlite( "makeTable.db" );

stat = db.makeTable( "create table users3 ( \
	account char PRIMARY KEY COLLATE NOCASE, \
	user_id char(42) default (getKUID()), \
	username char COLLATE NOCASE, \
	email char COLLATE NOCASE, \
	client_id char(42), \
	password char, \
 	`requests` int, \
	createdUser  DATETIME DEFAULT CURRENT_TIMESTAMP, \
	FOREIGN KEY(client_id) REFERENCES clientKeys(key) ON DELETE CASCADE, \
	UNIQUE KEY user_id_key(user_id), \
	UNIQUE KEY emailkey(email), \
	UNIQUE KEY user_name_key(username) \
)") 

        if( !stat ) console.log( "error?", db.error );

	var stat;
	stat = db.makeTable( 'create table game_state ( \
        	`config` char PRIMARY KEY, \
                `current_game` char, \
                ) ');
        if( !stat ) console.log( "error?", db.error );
	stat = db.makeTable( "create table game_sets ( \
        	`game_set_id` char PRIMARY KEY, \
                `created` DATETIME \
                ) ");
        
        if( !stat ) console.log( "error?", db.error );
	stat = db.makeTable( "create table games ( \
        	`game_id` char PRIMARY KEY, \
                `game_set_id` char, \
                `prior_game_id` char default NULL, \
                `next_game_id` char default NULL, \
		`order` int default 0, \
                `created` DATETIME, \
		FOREIGN KEY(game_set_id) REFERENCES game_sets(game_set_id) ON DELETE CASCADE, \
		FOREIGN KEY(prior_game_id) REFERENCES games(game_id) ON DELETE CASCADE, \
		FOREIGN KEY(next_game_id) REFERENCES games(game_id) ON DELETE CASCADE, \
		INDEX gameSetGames(game_set_id) \
                ) " );

        if( !stat ) console.log( "error?", db.error );
        stat = db.makeTable( "create table game_coins ( \
        	`game_coin_id` char PRIMARY KEY, \
                `game_id` char, \
                `seat` int, \
                `red` int, \
                `blue` int, \
		FOREIGN KEY(game_id) REFERENCES games(game_id) ON DELETE CASCADE, \
		UNIQUE KEY gameSeat(game_id,seat), \
		INDEX gameCoins(game_id) \
                )");
        if( !stat ) console.log( "error?", db.error );
                
	stat = db.makeTable( "create table game_calls ( \
        	`game_call_id` int PRIMARY KEY, \
        	`game_id` char, \
                `ball` int, \
                `order` int, \
		FOREIGN KEY(game_id) REFERENCES games(game_id) ON DELETE CASCADE, \
		INDEX gameCalls(game_id) \
                )");
        if( !stat ) console.log( "error?", db.error );
