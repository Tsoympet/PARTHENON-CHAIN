import React from 'react';
import {SafeAreaProvider} from 'react-native-safe-area-context';
import {Provider} from 'react-redux';
import {NavigationContainer} from '@react-navigation/native';
import {PersistGate} from 'redux-persist/integration/react';
import {store, persistor} from './src/store';
import {AppNavigator} from './src/navigation/AppNavigator';

function App(): JSX.Element {
  return (
    <Provider store={store}>
      <SafeAreaProvider>
        <PersistGate loading={null} persistor={persistor}>
          <NavigationContainer>
            <AppNavigator />
          </NavigationContainer>
        </PersistGate>
      </SafeAreaProvider>
    </Provider>
  );
}

export default App;
