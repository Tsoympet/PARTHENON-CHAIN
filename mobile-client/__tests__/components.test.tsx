/**
 * Component Tests
 */

import React from 'react';
import {render, fireEvent} from '@testing-library/react-native';
import {Button} from '../src/components/common/Button';

describe('Button Component', () => {
  it('renders correctly', () => {
    const {getByText} = render(
      <Button title="Test Button" onPress={() => {}} />
    );
    expect(getByText('Test Button')).toBeTruthy();
  });

  it('calls onPress when clicked', () => {
    const onPressMock = jest.fn();
    const {getByText} = render(
      <Button title="Click Me" onPress={onPressMock} />
    );
    
    fireEvent.press(getByText('Click Me'));
    expect(onPressMock).toHaveBeenCalled();
  });

  it('is disabled when disabled prop is true', () => {
    const onPressMock = jest.fn();
    const {getByText} = render(
      <Button title="Disabled" onPress={onPressMock} disabled />
    );
    
    const button = getByText('Disabled').parent;
    expect(button?.props.accessibilityState?.disabled).toBe(true);
  });

  it('shows loading indicator when loading', () => {
    const {queryByText} = render(
      <Button title="Loading" onPress={() => {}} loading />
    );
    
    // Text should not be visible when loading
    expect(queryByText('Loading')).toBeNull();
  });
});
