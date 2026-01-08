/**
 * Redux Middleware for logging actions
 */

import {Middleware} from '@reduxjs/toolkit';

export const loggerMiddleware: Middleware = store => next => action => {
  if (__DEV__) {
    console.log('Dispatching:', action.type);
    console.log('Previous State:', store.getState());
  }
  
  const result = next(action);
  
  if (__DEV__) {
    console.log('Next State:', store.getState());
  }
  
  return result;
};
