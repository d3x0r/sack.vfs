export class Event {
	cb:any;
	priority: number;
	set enableArrayArgs(value: any): void;
	get enableArrayArgs():any;
	#private;
}

export class Events {
    static "__#1@#log": boolean;
    static set log(arg: any);
    static on(evt: any, d: any, extra:any): any;
    static off(evt: any, d: any): void;
    on(evt: any, d: any, extra:any): any;
    off(evt: any, d: any): void;
    #private;
}
