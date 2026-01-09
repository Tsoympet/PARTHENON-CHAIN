/**
 * RPC Client for PARTHENON CHAIN (Drachma)
 * 
 * Provides a secure connection to Drachma nodes for wallet operations.
 * Supports authentication, request batching, and error handling.
 */

import axios, {AxiosInstance} from 'axios';

export interface RPCConfig {
  url: string;
  username?: string;
  password?: string;
  timeout?: number;
}

export interface RPCRequest {
  jsonrpc: string;
  id: string | number;
  method: string;
  params: any[];
}

export interface RPCResponse<T = any> {
  jsonrpc: string;
  id: string | number;
  result?: T;
  error?: {
    code: number;
    message: string;
  };
}

export class RPCClient {
  private client: AxiosInstance;
  private requestId = 0;

  constructor(private config: RPCConfig) {
    this.client = axios.create({
      baseURL: config.url,
      timeout: config.timeout || 30000,
      headers: {
        'Content-Type': 'application/json',
      },
      auth: config.username && config.password ? {
        username: config.username,
        password: config.password,
      } : undefined,
    });
  }

  /**
   * Execute a single RPC call
   */
  async call<T = any>(method: string, params: any[] = []): Promise<T> {
    const request: RPCRequest = {
      jsonrpc: '2.0',
      id: ++this.requestId,
      method,
      params,
    };

    try {
      const response = await this.client.post<RPCResponse<T>>('', request);
      
      if (response.data.error) {
        throw new Error(
          `RPC Error (${response.data.error.code}): ${response.data.error.message}`
        );
      }

      return response.data.result as T;
    } catch (error) {
      if (axios.isAxiosError(error)) {
        throw new Error(`Network error: ${error.message}`);
      }
      throw error;
    }
  }

  /**
   * Execute multiple RPC calls in a single request (batch)
   */
  async batchCall<T = any>(calls: Array<{method: string; params?: any[]}>): Promise<T[]> {
    const requests: RPCRequest[] = calls.map(call => ({
      jsonrpc: '2.0',
      id: ++this.requestId,
      method: call.method,
      params: call.params || [],
    }));

    try {
      const response = await this.client.post<RPCResponse<T>[]>('', requests);
      
      return response.data.map(res => {
        if (res.error) {
          throw new Error(
            `RPC Error (${res.error.code}): ${res.error.message}`
          );
        }
        return res.result as T;
      });
    } catch (error) {
      if (axios.isAxiosError(error)) {
        throw new Error(`Network error: ${error.message}`);
      }
      throw error;
    }
  }

  // Wallet RPC methods
  async getBalance(assetId?: string): Promise<number> {
    return this.call('getbalance', assetId ? [assetId] : []);
  }

  async getNewAddress(): Promise<string> {
    return this.call('getnewaddress');
  }

  async sendToAddress(address: string, amount: number, assetId?: string): Promise<string> {
    const params = [address, amount];
    if (assetId) params.push(assetId);
    return this.call('sendtoaddress', params);
  }

  async listTransactions(count = 10, skip = 0): Promise<any[]> {
    return this.call('listtransactions', ['*', count, skip]);
  }

  async getTransaction(txid: string): Promise<any> {
    return this.call('gettransaction', [txid]);
  }

  async getBlockchainInfo(): Promise<any> {
    return this.call('getblockchaininfo');
  }

  async getBlockTemplate(): Promise<any> {
    return this.call('getblocktemplate', [{rules: ['segwit']}]);
  }

  async submitBlockShare(payload: {
    jobId: string;
    nonce: number;
    hashRate: number;
  }): Promise<boolean> {
    return this.call('submitblock', [payload]);
  }

  async getStakingInfo(): Promise<any> {
    return this.call('getstakinginfo');
  }

  async validateAddress(address: string): Promise<{isvalid: boolean; address?: string}> {
    return this.call('validateaddress', [address]);
  }

  async estimateFee(blocks = 6): Promise<number> {
    return this.call('estimatefee', [blocks]);
  }

  // NFT methods (Layer 2)
  async listNFTs(owner?: string): Promise<any[]> {
    return this.call('list_nft', owner ? [owner] : []);
  }

  async mintNFT(metadata: any): Promise<string> {
    return this.call('mint_nft', [metadata]);
  }

  async transferNFT(tokenId: string, to: string): Promise<string> {
    return this.call('transfer_nft', [tokenId, to]);
  }
}

export default RPCClient;
