/**
 * A subscription, returned by `on()`.
 *
 * It carries the list it lives in, so `off(handle)` can splice it out directly
 * rather than searching by name and function.
 */
export class EventHandle {
    /** @param {EventHandler} cb */
    constructor(cb: EventHandler);
    /** @type {EventHandler|null} */
    cb: EventHandler | null;
    /** The handler list this registration belongs to. @type {EventHandle[]|null} */
    list: EventHandle[] | null;
    /** Higher runs earlier; equal priorities keep registration order. @type {number} */
    priority: number;
    /**
     * Whether an array payload is spread across this handler's parameters.
     *
     * Set false to receive the array whole as a single argument — the per-handler
     * escape hatch that Events2 does not have.
     *
     * @param {boolean} value
     */
    set enableArrayArgs(value: boolean);
    /** @returns {boolean} */
    get enableArrayArgs(): boolean;
    #private;
}
export class Events {
    /**
     * Enable debug logging for this class. Once enabled it cannot be disabled.
     * @param {boolean} value
     */
    static set log(value: boolean);
    /**
     * Whether the argument after a handler is treated as a priority.
     * @param {boolean} value
     */
    static set usePriority(value: boolean);
    /**
     * Subscribe to a static event.
     * @overload
     * @param {string} evt
     * @param {EventHandler} d
     * @param {number} [extra] priority; higher runs earlier, default 0
     * @returns {EventHandle}
     */
    static on(evt: string, d: EventHandler, extra?: number): EventHandle;
    /**
     * Dispatch a static event.
     * @overload
     * @param {string} evt
     * @param {any[]|Record<string,any>|string|number|boolean} d
     * @returns {any[]|undefined} every handler's return value, in call order
     */
    static on(evt: string, d: any[] | Record<string, any> | string | number | boolean): any[] | undefined;
    /**
     * Ask whether anything is listening.
     * @overload
     * @param {string} evt
     * @returns {boolean|undefined}
     */
    static on(evt: string): boolean | undefined;
    /**
     * Remove a static subscription by its handle.
     * @overload
     * @param {EventHandle} evt
     * @returns {void}
     */
    static off(evt: EventHandle): void;
    /**
     * Remove a static subscription by name and function.
     * @overload
     * @param {string} evt
     * @param {EventHandler} d
     * @returns {void}
     */
    static off(evt: string, d: EventHandler): void;
    /**
     * Whether the argument after a handler is treated as a priority.
     * @param {boolean} value
     */
    set usePriority(value: boolean);
    /**
     * Subscribe.
     * @overload
     * @param {string} evt
     * @param {EventHandler} d
     * @param {number} [extra] priority; higher runs earlier, default 0
     * @returns {EventHandle}
     */
    on(evt: string, d: EventHandler, extra?: number): EventHandle;
    /**
     * Dispatch. An array payload is spread across each handler's parameters
     * unless that handle has `enableArrayArgs = false`.
     * @overload
     * @param {string} evt
     * @param {any[]|Record<string,any>|string|number|boolean} d
     * @returns {any[]|undefined} every handler's return value, in call order
     */
    on(evt: string, d: any[] | Record<string, any> | string | number | boolean): any[] | undefined;
    /**
     * Ask whether anything is listening.
     * @overload
     * @param {string} evt
     * @returns {boolean|undefined}
     */
    on(evt: string): boolean | undefined;
    /**
     * Remove a subscription by its handle — the cheap path, since the handle
     * already knows which list it is in.
     * @overload
     * @param {EventHandle} evt
     * @returns {void}
     */
    off(evt: EventHandle): void;
    /**
     * Remove a subscription by name and function.
     * @overload
     * @param {string} evt
     * @param {EventHandler} d
     * @returns {void}
     */
    off(evt: string, d: EventHandler): void;
    #private;
}
/**
 * A handler.
 *
 * Dispatch passes the payload followed by the array of results returned by the
 * handlers that already ran, so a handler can see what came before it.  An
 * array payload is spread unless this handle has `enableArrayArgs = false`.
 */
export type EventHandler = (...args: any[]) => any;
/**
 * Per-class state, keyed by constructor.
 */
export type EventTypeState = {
    static_events: {
        [x: string]: EventHandle[];
    };
    usePriority: boolean;
    log: boolean;
};
