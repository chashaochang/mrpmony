declare module 'libmrp_napi.so' {
  export function init(options: {
    workDir: string;
    width: number;
    height: number;
    debug?: boolean;
  }): {
    ok: boolean;
    errorCode?: number;
    errorMessage?: string;
  };

  export function loadPackage(packagePath: string): {
    ok: boolean;
    errorCode?: number;
    errorMessage?: string;
  };

  export function start(): {
    ok: boolean;
    errorCode?: number;
    errorMessage?: string;
  };

  export function startSessionAsync(
    options: {
      workDir: string;
      width: number;
      height: number;
      debug?: boolean;
    },
    appId: string
  ): Promise<{
    ok: boolean;
    errorCode?: number;
    errorMessage?: string;
  }>;

  export function pause(): { ok: boolean; errorCode?: number; errorMessage?: string };
  export function resume(): { ok: boolean; errorCode?: number; errorMessage?: string };
  export function release(): { ok: boolean; errorCode?: number; errorMessage?: string };
  export function releaseAsync(): Promise<{ ok: boolean; errorCode?: number; errorMessage?: string }>;
  export function gunzip(data: ArrayBuffer): ArrayBuffer;

  export function sendInput(event: {
    type: 'key' | 'touch';
    action: 'down' | 'up' | 'move' | 'tap';
    keyCode?: number;
    x?: number;
    y?: number;
    timestamp?: number;
  }): {
    ok: boolean;
    errorCode?: number;
    errorMessage?: string;
  };

  export function pullFrame(): {
    ok: boolean;
    hasNewFrame: boolean;
    exited?: boolean;
    width: number;
    height: number;
    pixelFormat: 'RGBA_8888';
    buffer?: ArrayBuffer;
    frameId?: number;
    errorCode?: number;
    errorMessage?: string;
  };

  export function pullEditRequest(): {
    ok: boolean;
    hasRequest: boolean;
    requestId?: number;
    handle?: number;
    type?: number;
    maxSize?: number;
    title?: string;
    text?: string;
    errorCode?: number;
    errorMessage?: string;
  };

  export function submitEditResult(payload: {
    requestId: number;
    confirmed: boolean;
    text: string;
  }): {
    ok: boolean;
    errorCode?: number;
    errorMessage?: string;
  };
}
